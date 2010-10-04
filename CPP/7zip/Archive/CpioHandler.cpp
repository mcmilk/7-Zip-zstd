// CpioHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

#include "Common/ItemNameUtils.h"

namespace NArchive {
namespace NCpio {

namespace NFileHeader
{
  namespace NMagic
  {
    const char *kMagic1 = "070701";
    const char *kMagic2 = "070702";
    const char *kMagic3 = "070707";
    const char *kEndName = "TRAILER!!!";

    const Byte kMagicForRecord2[2] = { 0xC7, 0x71 };
  }

  const UInt32 kRecord2Size = 26;
  /*
  struct CRecord2
  {
    unsigned short c_magic;
    short c_dev;
    unsigned short c_ino;
    unsigned short c_mode;
    unsigned short c_uid;
    unsigned short c_gid;
    unsigned short c_nlink;
    short c_rdev;
    unsigned short c_mtimes[2];
    unsigned short c_namesize;
    unsigned short c_filesizes[2];
  };
  */
 
  const UInt32 kRecordSize = 110;
  /*
  struct CRecord
  {
    char Magic[6];  // "070701" for "new" portable format, "070702" for CRC format
    char inode[8];
    char Mode[8];
    char UID[8];
    char GID[8];
    char nlink[8];
    char mtime[8];
    char Size[8]; // must be 0 for FIFOs and directories
    char DevMajor[8];
    char DevMinor[8];
    char RDevMajor[8];  //only valid for chr and blk special files
    char RDevMinor[8];  //only valid for chr and blk special files
    char NameSize[8]; // count includes terminating NUL in pathname
    char ChkSum[8];  // 0 for "new" portable format; for CRC format the sum of all the bytes in the file
    bool CheckMagic() const
    { return memcmp(Magic, NMagic::kMagic1, 6) == 0 ||
             memcmp(Magic, NMagic::kMagic2, 6) == 0; };
  };
  */

  const UInt32 kOctRecordSize = 76;
 
}

struct CItem
{
  AString Name;
  UInt32 inode;
  UInt32 Mode;
  UInt32 UID;
  UInt32 GID;
  UInt32 Size;
  UInt32 MTime;

  // char LinkFlag;
  // AString LinkName; ?????
  char Magic[8];
  UInt32 NumLinks;
  UInt32 DevMajor;
  UInt32 DevMinor;
  UInt32 RDevMajor;
  UInt32 RDevMinor;
  UInt32 ChkSum;

  UInt32 Align;

  bool IsDir() const { return (Mode & 0170000) == 0040000; }
};

class CItemEx: public CItem
{
public:
  UInt64 HeaderPosition;
  UInt32 HeaderSize;
  UInt64 GetDataPosition() const { return HeaderPosition + HeaderSize; };
};

const UInt32 kMaxBlockSize = NFileHeader::kRecordSize;

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;

  UInt16 _blockSize;
  Byte _block[kMaxBlockSize];
  UInt32 _blockPos;
  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
  
  bool ReadNumber(UInt32 &resultValue);
  bool ReadOctNumber(int size, UInt32 &resultValue);

  HRESULT ReadBytes(void *data, UInt32 size, UInt32 &processedSize);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT Skip(UInt64 numBytes);
  HRESULT SkipDataRecords(UInt64 dataSize, UInt32 align);
};

HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
{
  size_t realProcessedSize = size;
  RINOK(ReadStream(m_Stream, data, &realProcessedSize));
  processedSize = (UInt32)realProcessedSize;
  m_Position += processedSize;
  return S_OK;
}

Byte CInArchive::ReadByte()
{
  if (_blockPos >= _blockSize)
    throw "Incorrect cpio archive";
  return _block[_blockPos++];
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  m_Stream = inStream;
  return S_OK;
}

bool CInArchive::ReadNumber(UInt32 &resultValue)
{
  resultValue = 0;
  for (int i = 0; i < 8; i++)
  {
    char c = char(ReadByte());
    int d;
    if (c >= '0' && c <= '9')
      d = c - '0';
    else if (c >= 'A' && c <= 'F')
      d = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f')
      d = 10 + c - 'a';
    else
      return false;
    resultValue *= 0x10;
    resultValue += d;
  }
  return true;
}

static bool OctalToNumber(const char *s, UInt64 &res)
{
  const char *end;
  res = ConvertOctStringToUInt64(s, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *s, UInt32 &res)
{
  UInt64 res64;
  if (!OctalToNumber(s, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

bool CInArchive::ReadOctNumber(int size, UInt32 &resultValue)
{
  char sz[32 + 4];
  int i;
  for (i = 0; i < size && i < 32; i++)
    sz[i] = (char)ReadByte();
  sz[i] = 0;
  return OctalToNumber32(sz, resultValue);
}

#define GetFromHex(y) { if (!ReadNumber(y)) return S_FALSE; }
#define GetFromOct6(y) { if (!ReadOctNumber(6, y)) return S_FALSE; }
#define GetFromOct11(y) { if (!ReadOctNumber(11, y)) return S_FALSE; }

static unsigned short ConvertValue(unsigned short value, bool convert)
{
  if (!convert)
    return value;
  return (unsigned short)((((unsigned short)(value & 0xFF)) << 8) | (value >> 8));
}

static UInt32 GetAlignedSize(UInt32 size, UInt32 align)
{
  while ((size & (align - 1)) != 0)
    size++;
  return size;
}


HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  filled = false;

  UInt32 processedSize;
  item.HeaderPosition = m_Position;

  _blockSize = kMaxBlockSize;
  RINOK(ReadBytes(_block, 2, processedSize));
  if (processedSize != 2)
    return S_FALSE;
  _blockPos = 0;

  UInt32 nameSize;

  bool oldBE =
    _block[0] == NFileHeader::NMagic::kMagicForRecord2[1] &&
    _block[1] == NFileHeader::NMagic::kMagicForRecord2[0];

  bool binMode = (_block[0] == NFileHeader::NMagic::kMagicForRecord2[0] &&
    _block[1] == NFileHeader::NMagic::kMagicForRecord2[1]) ||
    oldBE;

  if (binMode)
  {
    RINOK(ReadBytes(_block + 2, NFileHeader::kRecord2Size - 2, processedSize));
    if (processedSize != NFileHeader::kRecord2Size - 2)
      return S_FALSE;
    item.Align = 2;
    _blockPos = 2;
    item.DevMajor = 0;
    item.DevMinor = ConvertValue(ReadUInt16(), oldBE);
    item.inode = ConvertValue(ReadUInt16(), oldBE);
    item.Mode = ConvertValue(ReadUInt16(), oldBE);
    item.UID = ConvertValue(ReadUInt16(), oldBE);
    item.GID = ConvertValue(ReadUInt16(), oldBE);
    item.NumLinks = ConvertValue(ReadUInt16(), oldBE);
    item.RDevMajor =0;
    item.RDevMinor = ConvertValue(ReadUInt16(), oldBE);
    UInt16 timeHigh = ConvertValue(ReadUInt16(), oldBE);
    UInt16 timeLow = ConvertValue(ReadUInt16(), oldBE);
    item.MTime = (UInt32(timeHigh) << 16) + timeLow;
    nameSize = ConvertValue(ReadUInt16(), oldBE);
    UInt16 sizeHigh = ConvertValue(ReadUInt16(), oldBE);
    UInt16 sizeLow = ConvertValue(ReadUInt16(), oldBE);
    item.Size = (UInt32(sizeHigh) << 16) + sizeLow;

    item.ChkSum = 0;
    item.HeaderSize = GetAlignedSize(
        nameSize + NFileHeader::kRecord2Size, item.Align);
    nameSize = item.HeaderSize - NFileHeader::kRecord2Size;
  }
  else
  {
    RINOK(ReadBytes(_block + 2, 4, processedSize));
    if (processedSize != 4)
      return S_FALSE;

    bool magicOK =
        memcmp(_block, NFileHeader::NMagic::kMagic1, 6) == 0 ||
        memcmp(_block, NFileHeader::NMagic::kMagic2, 6) == 0;
    _blockPos = 6;
    if (magicOK)
    {
      RINOK(ReadBytes(_block + 6, NFileHeader::kRecordSize - 6, processedSize));
      if (processedSize != NFileHeader::kRecordSize - 6)
        return S_FALSE;
      item.Align = 4;

      GetFromHex(item.inode);
      GetFromHex(item.Mode);
      GetFromHex(item.UID);
      GetFromHex(item.GID);
      GetFromHex(item.NumLinks);
      UInt32 mTime;
      GetFromHex(mTime);
      item.MTime = mTime;
      GetFromHex(item.Size);
      GetFromHex(item.DevMajor);
      GetFromHex(item.DevMinor);
      GetFromHex(item.RDevMajor);
      GetFromHex(item.RDevMinor);
      GetFromHex(nameSize);
      GetFromHex(item.ChkSum);
      item.HeaderSize = GetAlignedSize(
          nameSize + NFileHeader::kRecordSize, item.Align);
      nameSize = item.HeaderSize - NFileHeader::kRecordSize;
    }
    else
    {
      if (!memcmp(_block, NFileHeader::NMagic::kMagic3, 6) == 0)
        return S_FALSE;
      RINOK(ReadBytes(_block + 6, NFileHeader::kOctRecordSize - 6, processedSize));
      if (processedSize != NFileHeader::kOctRecordSize - 6)
        return S_FALSE;
      item.Align = 1;
      item.DevMajor = 0;
      GetFromOct6(item.DevMinor);
      GetFromOct6(item.inode);
      GetFromOct6(item.Mode);
      GetFromOct6(item.UID);
      GetFromOct6(item.GID);
      GetFromOct6(item.NumLinks);
      item.RDevMajor = 0;
      GetFromOct6(item.RDevMinor);
      UInt32 mTime;
      GetFromOct11(mTime);
      item.MTime = mTime;
      GetFromOct6(nameSize);
      GetFromOct11(item.Size);  // ?????
      item.HeaderSize = GetAlignedSize(
          nameSize + NFileHeader::kOctRecordSize, item.Align);
      nameSize = item.HeaderSize - NFileHeader::kOctRecordSize;
    }
  }
  if (nameSize == 0 || nameSize >= (1 << 27))
    return E_FAIL;
  RINOK(ReadBytes(item.Name.GetBuffer(nameSize), nameSize, processedSize));
  if (processedSize != nameSize)
    return E_FAIL;
  item.Name.ReleaseBuffer();
  if (strcmp(item.Name, NFileHeader::NMagic::kEndName) == 0)
    return S_OK;
  filled = true;
  return S_OK;
}

HRESULT CInArchive::Skip(UInt64 numBytes)
{
  UInt64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkipDataRecords(UInt64 dataSize, UInt32 align)
{
  while ((dataSize & (align - 1)) != 0)
    dataSize++;
  return Skip(dataSize);
}


class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _stream;
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

/*
enum
{
  kpidinode = kpidUserDefined,
  kpidiChkSum
};
*/

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidPosixAttrib, VT_UI4},
  // { L"inode", kpidinode, VT_UI4}
  // { L"CheckSum", kpidiChkSum, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  // try
  {
    CInArchive archive;

    UInt64 endPos = 0;
    bool needSetTotal = true;

    if (callback != NULL)
    {
      RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
      RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
    }

    RINOK(archive.Open(stream));

    _items.Clear();

    for (;;)
    {
      CItemEx item;
      bool filled;
      HRESULT result = archive.GetNextItem(filled, item);
      if (result == S_FALSE)
        return S_FALSE;
      if (result != S_OK)
        return S_FALSE;
      if (!filled)
        break;
      _items.Add(item);
      archive.SkipDataRecords(item.Size, item.Align);
      if (callback != NULL)
      {
        if (needSetTotal)
        {
          RINOK(callback->SetTotal(NULL, &endPos));
          needSetTotal = false;
        }
        if (_items.Size() % 100 == 0)
        {
          UInt64 numFiles = _items.Size();
          UInt64 numBytes = item.HeaderPosition;
          RINOK(callback->SetCompleted(&numFiles, &numBytes));
        }
      }
    }
    if (_items.Size() == 0)
      return S_FALSE;

    _stream = stream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _items.Clear();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItemEx &item = _items[index];

  switch(propID)
  {
    case kpidPath: prop = NItemName::GetOSName(MultiByteToUnicodeString(item.Name, CP_OEMCP)); break;
    case kpidIsDir: prop = item.IsDir(); break;
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)item.Size;
      break;
    case kpidMTime:
    {
      if (item.MTime != 0)
      {
        FILETIME utc;
        NWindows::NTime::UnixTimeToFileTime(item.MTime, utc);
        prop = utc;
      }
      break;
    }
    case kpidPosixAttrib: prop = item.Mode; break;
    /*
    case kpidinode:  prop = item.inode; break;
    case kpidiChkSum:  prop = item.ChkSum; break;
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_stream);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> outStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    currentTotalSize += item.Size;
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    if (testMode)
    {
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    RINOK(_stream->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
    streamSpec->Init(item.Size);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStream.Release();
    RINOK(extractCallback->SetOperationResult((copyCoderSpec->TotalSize == item.Size) ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  const CItemEx &item = _items[index];
  return CreateLimitedInStream(_stream, item.GetDataPosition(), item.Size, stream);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NCpio::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Cpio", L"cpio", 0, 0xED, { 0 }, 0, false, CreateArc, 0 };

REGISTER_ARC(Cpio)

}}
