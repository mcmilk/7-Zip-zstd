// ArjHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/ArjDecoder1.h"
#include "../Compress/ArjDecoder2.h"
#include "../Compress/CopyCoder.h"

#include "Common/ItemNameUtils.h"
#include "Common/OutStreamWithCRC.h"

using namespace NWindows;

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)

namespace NArchive {
namespace NArj {

const int kBlockSizeMin = 30;
const int kBlockSizeMax = 2600;

namespace NSignature
{
  const Byte kSig0 = 0x60;
  const Byte kSig1 = 0xEA;
}

namespace NFileHeader
{
  namespace NCompressionMethod
  {
    enum
    {
      kStored = 0,
      kCompressed1a = 1,
      kCompressed1b = 2,
      kCompressed1c = 3,
      kCompressed2 = 4,
      kNoDataNoCRC = 8,
      kNoData = 9
    };
  }

  namespace NFileType
  {
    enum
    {
      kBinary = 0,
      k7BitText = 1,
      kArchiveHeader = 2,
      kDirectory = 3,
      kVolumeLablel = 4,
      kChapterLabel = 5
    };
  }
  
  namespace NFlags
  {
    const Byte kGarbled = 1;
    const Byte kVolume = 4;
    const Byte kExtFile = 8;
    const Byte kPathSym = 0x10;
    const Byte kBackup = 0x20;
  }

  namespace NHostOS
  {
    enum EEnum
    {
      kMSDOS = 0,  // filesystem used by MS-DOS, OS/2, Win32
          // pkarj 2.50 (FAT / VFAT / FAT32 file systems)
      kPRIMOS,
      kUnix,
      kAMIGA,
      kMac,
      kOS_2,
      kAPPLE_GS,
      kAtari_ST,
      kNext,
      kVAX_VMS,
      kWIN95
    };
  }
}

struct CArchiveHeader
{
  // Byte ArchiverVersion;
  // Byte ExtractVersion;
  Byte HostOS;
  // Byte Flags;
  // Byte SecuryVersion;
  // Byte FileType;
  // Byte Reserved;
  UInt32 CTime;
  UInt32 MTime;
  UInt32 ArchiveSize;
  // UInt32 SecurityEnvelopeFilePosition;
  // UInt16 FilespecPositionInFilename;
  // UInt16 LengthOfSecurityEnvelopeSata;
  // Byte EncryptionVersion;
  // Byte LastChapter;
  AString Name;
  AString Comment;
  
  HRESULT Parse(const Byte *p, unsigned size);
};

static HRESULT ReadString(const Byte *p, unsigned &size, AString &res)
{
  AString s;
  for (unsigned i = 0; i < size;)
  {
    char c = (char)p[i++];
    if (c == 0)
    {
      size = i;
      res = s;
      return S_OK;
    }
    s += c;
  }
  return S_FALSE;
}

HRESULT CArchiveHeader::Parse(const Byte *p, unsigned size)
{
  if (size < kBlockSizeMin)
    return S_FALSE;
  Byte firstHeaderSize = p[0];
  if (firstHeaderSize > size)
    return S_FALSE;
  // ArchiverVersion = p[1];
  // ExtractVersion = p[2];
  HostOS = p[3];
  // Flags = p[4];
  // SecuryVersion = p[5];
  if (p[6] != NFileHeader::NFileType::kArchiveHeader)
    return S_FALSE;
  // Reserved = p[7];
  CTime = Get32(p + 8);
  MTime = Get32(p + 12);
  ArchiveSize = Get32(p + 16);
  // SecurityEnvelopeFilePosition = Get32(p + 20);
  // UInt16 filespecPositionInFilename = Get16(p + 24);
  // LengthOfSecurityEnvelopeSata = Get16(p + 26);
  // EncryptionVersion = p[28];
  // LastChapter = p[29];
  unsigned pos = firstHeaderSize;
  unsigned size1 = size - pos;
  RINOK(ReadString(p + pos, size1, Name));
  pos += size1;
  size1 = size - pos;
  RINOK(ReadString(p + pos, size1, Comment));
  pos += size1;
  return S_OK;
}

struct CItem
{
  AString Name;
  AString Comment;

  UInt32 MTime;
  UInt32 PackSize;
  UInt32 Size;
  UInt32 FileCRC;
  UInt32 SplitPos;

  Byte Version;
  Byte ExtractVersion;
  Byte HostOS;
  Byte Flags;
  Byte Method;
  Byte FileType;

  // UInt16 FilespecPositionInFilename;
  UInt16 FileAccessMode;
  // Byte FirstChapter;
  // Byte LastChapter;
  
  UInt64 DataPosition;
  
  bool IsEncrypted() const { return (Flags & NFileHeader::NFlags::kGarbled) != 0; }
  bool IsDir() const { return (FileType == NFileHeader::NFileType::kDirectory); }
  bool IsSplitAfter() const { return (Flags & NFileHeader::NFlags::kVolume) != 0; }
  bool IsSplitBefore() const { return (Flags & NFileHeader::NFlags::kExtFile) != 0; }
  UInt32 GetWinAttributes() const
  {
    UInt32 winAtrributes;
    switch(HostOS)
    {
      case NFileHeader::NHostOS::kMSDOS:
      case NFileHeader::NHostOS::kWIN95:
        winAtrributes = FileAccessMode;
        break;
      default:
        winAtrributes = 0;
    }
    if (IsDir())
      winAtrributes |= FILE_ATTRIBUTE_DIRECTORY;
    return winAtrributes;
  }

  HRESULT Parse(const Byte *p, unsigned size);
};

HRESULT CItem::Parse(const Byte *p, unsigned size)
{
  if (size < kBlockSizeMin)
    return S_FALSE;

  Byte firstHeaderSize = p[0];

  Version = p[1];
  ExtractVersion = p[2];
  HostOS = p[3];
  Flags = p[4];
  Method = p[5];
  FileType = p[6];
  // Reserved = p[7];
  MTime = Get32(p + 8);
  PackSize = Get32(p + 12);
  Size = Get32(p + 16);
  FileCRC = Get32(p + 20);
  // FilespecPositionInFilename = Get16(p + 24);
  FileAccessMode = Get16(p + 26);
  // FirstChapter = p[28];
  // FirstChapter = p[29];

  SplitPos = 0;
  if (IsSplitBefore() && firstHeaderSize >= 34)
    SplitPos = Get32(p + 30);

  unsigned pos = firstHeaderSize;
  unsigned size1 = size - pos;
  RINOK(ReadString(p + pos, size1, Name));
  pos += size1;
  size1 = size - pos;
  RINOK(ReadString(p + pos, size1, Comment));
  pos += size1;

  return S_OK;
}

struct CInArchiveException
{
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kCRCError,
    kIncorrectArchive
  }
  Cause;
  CInArchiveException(CCauseType cause): Cause(cause) {};
};

class CInArchive
{
  UInt32 _blockSize;
  Byte _block[kBlockSizeMax + 4];
  
  HRESULT ReadBlock(bool &filled);
  HRESULT ReadSignatureAndBlock(bool &filled);
  HRESULT SkipExtendedHeaders();

  HRESULT SafeReadBytes(void *data, UInt32 size);
    
public:
  CArchiveHeader Header;

  IInStream *Stream;
  IArchiveOpenCallback *Callback;
  UInt64 NumFiles;
  UInt64 NumBytes;

  HRESULT Open(const UInt64 *searchHeaderSizeLimit);
  HRESULT GetNextItem(bool &filled, CItem &item);
};

static inline bool TestMarkerCandidate(const Byte *p, unsigned maxSize)
{
  if (p[0] != NSignature::kSig0 || p[1] != NSignature::kSig1)
    return false;
  UInt32 blockSize = Get16(p + 2);
  p += 4;
  if (p[6] != NFileHeader::NFileType::kArchiveHeader ||
      p[0] > blockSize ||
      maxSize < 2 + 2 + blockSize + 4 ||
      blockSize < kBlockSizeMin || blockSize > kBlockSizeMax ||
      p[28] > 8) // EncryptionVersion
    return false;
  // return (Get32(p + blockSize) == CrcCalc(p, blockSize));
  return true;
}

static HRESULT FindAndReadMarker(ISequentialInStream *stream, const UInt64 *searchHeaderSizeLimit, UInt64 &position)
{
  position = 0;

  const int kMarkerSizeMin = 2 + 2 + kBlockSizeMin + 4;
  const int kMarkerSizeMax = 2 + 2 + kBlockSizeMax + 4;

  CByteBuffer byteBuffer;
  const UInt32 kBufSize = 1 << 16;
  byteBuffer.SetCapacity(kBufSize);
  Byte *buf = byteBuffer;

  size_t processedSize = kMarkerSizeMax;
  RINOK(ReadStream(stream, buf, &processedSize));
  if (processedSize < kMarkerSizeMin)
    return S_FALSE;
  if (TestMarkerCandidate(buf, (unsigned)processedSize))
    return S_OK;

  UInt32 numBytesPrev = (UInt32)processedSize - 1;
  memmove(buf, buf + 1, numBytesPrev);
  UInt64 curTestPos = 1;
  for (;;)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos > *searchHeaderSizeLimit)
        return S_FALSE;
    processedSize = kBufSize - numBytesPrev;
    RINOK(ReadStream(stream, buf + numBytesPrev, &processedSize));
    UInt32 numBytesInBuffer = numBytesPrev + (UInt32)processedSize;
    if (numBytesInBuffer < kMarkerSizeMin)
      return S_FALSE;
    UInt32 numTests = numBytesInBuffer - kMarkerSizeMin + 1;
    UInt32 pos;
    for (pos = 0; pos < numTests; pos++)
    {
      for (; buf[pos] != NSignature::kSig0 && pos < numTests; pos++);
      if (pos == numTests)
        break;
      if (TestMarkerCandidate(buf + pos, numBytesInBuffer - pos))
      {
        position = curTestPos + pos;
        return S_OK;
      }
    }
    curTestPos += pos;
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buf, buf + numTests, numBytesPrev);
  }
}

HRESULT CInArchive::SafeReadBytes(void *data, UInt32 size)
{
  size_t processed = size;
  RINOK(ReadStream(Stream, data, &processed));
  if (processed != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

HRESULT CInArchive::ReadBlock(bool &filled)
{
  filled = false;
  Byte buf[2];
  RINOK(SafeReadBytes(buf, 2));
  _blockSize = Get16(buf);
  if (_blockSize == 0)
    return S_OK;
  if (_blockSize > kBlockSizeMax)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  RINOK(SafeReadBytes(_block, _blockSize + 4));
  NumBytes += _blockSize + 6;
  if (Get32(_block + _blockSize) != CrcCalc(_block, _blockSize))
    throw CInArchiveException(CInArchiveException::kCRCError);
  filled = true;
  return S_OK;
}

HRESULT CInArchive::ReadSignatureAndBlock(bool &filled)
{
  Byte id[2];
  RINOK(SafeReadBytes(id, 2));
  if (id[0] != NSignature::kSig0 || id[1] != NSignature::kSig1)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  return ReadBlock(filled);
}

HRESULT CInArchive::SkipExtendedHeaders()
{
  for (UInt32 i = 0;; i++)
  {
    bool filled;
    RINOK(ReadBlock(filled));
    if (!filled)
      return S_OK;
    if (Callback && (i & 0xFF) == 0)
      RINOK(Callback->SetCompleted(&NumFiles, &NumBytes));
  }
}

HRESULT CInArchive::Open(const UInt64 *searchHeaderSizeLimit)
{
  UInt64 position = 0;
  RINOK(FindAndReadMarker(Stream, searchHeaderSizeLimit, position));
  RINOK(Stream->Seek(position, STREAM_SEEK_SET, NULL));
  bool filled;
  RINOK(ReadSignatureAndBlock(filled));
  if (!filled)
    return S_FALSE;
  RINOK(Header.Parse(_block, _blockSize));
  return SkipExtendedHeaders();
}

HRESULT CInArchive::GetNextItem(bool &filled, CItem &item)
{
  RINOK(ReadSignatureAndBlock(filled));
  if (!filled)
    return S_OK;
  filled = false;
  RINOK(item.Parse(_block, _blockSize));
  /*
  UInt32 extraData;
  if ((header.Flags & NFileHeader::NFlags::kExtFile) != 0)
    extraData = GetUi32(_block + pos);
  */

  RINOK(SkipExtendedHeaders());
  filled = true;
  return S_OK;
}

class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

  HRESULT Open2(IInStream *inStream, const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *callback);
private:
  CInArchive _archive;
  CObjectVector<CItem> _items;
  CMyComPtr<IInStream> _stream;
};

const wchar_t *kHostOS[] =
{
  L"MSDOS",
  L"PRIMOS",
  L"UNIX",
  L"AMIGA",
  L"MAC",
  L"OS/2",
  L"APPLE GS",
  L"ATARI ST",
  L"NEXT",
  L"VAX VMS",
  L"WIN95"
};

const wchar_t *kUnknownOS = L"Unknown";

const int kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

STATPROPSTG kArcProps[] =
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidCTime, VT_BSTR},
  { NULL, kpidMTime, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidComment, VT_BSTR}
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI4},
  { NULL, kpidPosition, VT_UI8},
  { NULL, kpidPackSize, VT_UI4},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},
  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidCRC, VT_UI4},
  { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidComment, VT_BSTR}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static void SetTime(UInt32 dosTime, NWindows::NCOM::CPropVariant &prop)
{
  if (dosTime == 0)
    return;
  FILETIME localFileTime, utc;
  if (NTime::DosTimeToFileTime(dosTime, localFileTime))
  {
    if (!LocalFileTimeToFileTime(&localFileTime, &utc))
      utc.dwHighDateTime = utc.dwLowDateTime = 0;
  }
  else
    utc.dwHighDateTime = utc.dwLowDateTime = 0;
  prop = utc;
}

static void SetHostOS(Byte hostOS, NWindows::NCOM::CPropVariant &prop)
{
  prop = hostOS < kNumHostOSes ? kHostOS[hostOS] : kUnknownOS;
}

static void SetUnicodeString(const AString &s, NWindows::NCOM::CPropVariant &prop)
{
  if (!s.IsEmpty())
    prop = MultiByteToUnicodeString(s, CP_OEMCP);
}
 
STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidName:  SetUnicodeString(_archive.Header.Name, prop); break;
    case kpidCTime:  SetTime(_archive.Header.CTime, prop); break;
    case kpidMTime:  SetTime(_archive.Header.MTime, prop); break;
    case kpidHostOS:  SetHostOS(_archive.Header.HostOS, prop); break;
    case kpidComment:  SetUnicodeString(_archive.Header.Comment, prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
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
  const CItem &item = _items[index];
  switch(propID)
  {
    case kpidPath:  prop = NItemName::GetOSName(MultiByteToUnicodeString(item.Name, CP_OEMCP)); break;
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:  prop = item.Size; break;
    case kpidPackSize:  prop = item.PackSize; break;
    case kpidPosition:  if (item.IsSplitBefore() || item.IsSplitAfter()) prop = (UInt64)item.SplitPos; break;
    case kpidAttrib:  prop = item.GetWinAttributes(); break;
    case kpidEncrypted:  prop = item.IsEncrypted(); break;
    case kpidCRC:  prop = item.FileCRC; break;
    case kpidMethod:  prop = item.Method; break;
    case kpidHostOS:  SetHostOS(item.HostOS, prop); break;
    case kpidMTime:  SetTime(item.MTime, prop); break;
    case kpidComment:  SetUnicodeString(item.Comment, prop); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *inStream, const UInt64 *maxCheckStartPosition,
      IArchiveOpenCallback *callback)
{
  Close();
  
  UInt64 endPos = 0;
  if (callback != NULL)
  {
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPos));
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
  }
  
  _archive.Stream = inStream;
  _archive.Callback = callback;
  _archive.NumFiles = _archive.NumBytes = 0;

  RINOK(_archive.Open(maxCheckStartPosition));
  if (callback != NULL)
    RINOK(callback->SetTotal(NULL, &endPos));
  for (;;)
  {
    CItem item;
    bool filled;


    RINOK(_archive.GetNextItem(filled, item));
    
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &item.DataPosition));
    
    if (!filled)
      break;
    _items.Add(item);
    
    if (inStream->Seek(item.PackSize, STREAM_SEEK_CUR, NULL) != S_OK)
      throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);

    _archive.NumFiles = _items.Size();
    _archive.NumBytes = item.DataPosition;
    
    if (callback != NULL && _items.Size() % 100 == 0)
    {
      RINOK(callback->SetCompleted(&_archive.NumFiles, &_archive.NumBytes));
    }
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  HRESULT res;
  try
  {
    res = Open2(inStream, maxCheckStartPosition, callback);
    if (res == S_OK)
    {
      _stream = inStream;
      return S_OK;
    }
  }
  catch(const CInArchiveException &) { res = S_FALSE; }
  Close();
  return res;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _items.Clear();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  UInt64 totalUnpacked = 0, totalPacked = 0;
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CItem &item = _items[allFilesMode ? i : indices[i]];
    totalUnpacked += item.Size;
    totalPacked += item.PackSize;
  }
  extractCallback->SetTotal(totalUnpacked);

  totalUnpacked = totalPacked = 0;
  UInt64 curUnpacked, curPacked;
  
  CMyComPtr<ICompressCoder> arj1Decoder;
  CMyComPtr<ICompressCoder> arj2Decoder;
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *inStreamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(inStreamSpec);
  inStreamSpec->SetStream(_stream);

  for (i = 0; i < numItems; i++, totalUnpacked += curUnpacked, totalPacked += curPacked)
  {
    lps->InSize = totalPacked;
    lps->OutSize = totalUnpacked;
    RINOK(lps->SetCur());

    curUnpacked = curPacked = 0;

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (item.IsDir())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      }
      continue;
    }

    if (!testMode && !realOutStream)
      continue;

    RINOK(extractCallback->PrepareOperation(askMode));
    curUnpacked = item.Size;
    curPacked = item.PackSize;

    {
      COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
      CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->SetStream(realOutStream);
      realOutStream.Release();
      outStreamSpec->Init();
  
      inStreamSpec->Init(item.PackSize);
      
      UInt64 pos;
      _stream->Seek(item.DataPosition, STREAM_SEEK_SET, &pos);

      HRESULT result = S_OK;
      Int32 opRes = NExtract::NOperationResult::kOK;

      if (item.IsEncrypted())
        opRes = NExtract::NOperationResult::kUnSupportedMethod;
      else
      {
        switch(item.Method)
        {
          case NFileHeader::NCompressionMethod::kStored:
          {
            result = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
            if (result == S_OK && copyCoderSpec->TotalSize != item.PackSize)
              result = S_FALSE;
            break;
          }
          case NFileHeader::NCompressionMethod::kCompressed1a:
          case NFileHeader::NCompressionMethod::kCompressed1b:
          case NFileHeader::NCompressionMethod::kCompressed1c:
          {
            if (!arj1Decoder)
              arj1Decoder = new NCompress::NArj::NDecoder1::CCoder;
            result = arj1Decoder->Code(inStream, outStream, NULL, &curUnpacked, progress);
            break;
          }
          case NFileHeader::NCompressionMethod::kCompressed2:
          {
            if (!arj2Decoder)
              arj2Decoder = new NCompress::NArj::NDecoder2::CCoder;
            result = arj2Decoder->Code(inStream, outStream, NULL, &curUnpacked, progress);
            break;
          }
          default:
            opRes = NExtract::NOperationResult::kUnSupportedMethod;
        }
      }
      if (opRes == NExtract::NOperationResult::kOK)
      {
        if (result == S_FALSE)
          opRes = NExtract::NOperationResult::kDataError;
        else
        {
          RINOK(result);
          opRes = (outStreamSpec->GetCRC() == item.FileCRC) ?
              NExtract::NOperationResult::kOK:
              NExtract::NOperationResult::kCRCError;
        }
      }
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(opRes));
    }
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"Arj", L"arj", 0, 4, { 0x60, 0xEA }, 2, false, CreateArc, 0 };

REGISTER_ARC(Arj)

}}
