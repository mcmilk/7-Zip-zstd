// DebHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/NewHandler.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/Copy/CopyCoder.h"

#include "Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NDeb {

namespace NHeader
{
  const int kSignatureLen = 8;
  
  const char *kSignature  = "!<arch>\n";

  const int kNameSize = 16;
  const int kTimeSize = 12;
  const int kModeSize = 8;
  const int kSizeSize = 10;

  /*
  struct CHeader
  {
    char Name[kNameSize];
    char MTime[kTimeSize];
    char Number0[6];
    char Number1[6];
    char Mode[kModeSize];
    char Size[kSizeSize];
    char Quote;
    char NewLine;
  };
  */
  const int kHeaderSize = kNameSize + kTimeSize + 6 + 6 + kModeSize + kSizeSize + 1 + 1;
}

class CItem
{
public:
  AString Name;
  UInt64 Size;
  UInt32 MTime;
  UInt32 Mode;
};

class CItemEx: public CItem
{
public:
  UInt64 HeaderPosition;
  UInt64 GetDataPosition() const { return HeaderPosition + NHeader::kHeaderSize; };
  // UInt64 GetFullSize() const { return NFileHeader::kRecordSize + Size; };
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  UInt64 m_Position;
  
  HRESULT GetNextItemReal(bool &filled, CItemEx &itemInfo);
  HRESULT Skeep(UInt64 numBytes);
public:
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItemEx &itemInfo);
  HRESULT SkeepData(UInt64 dataSize);
};

HRESULT CInArchive::Open(IInStream *inStream)
{
  RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  char signature[NHeader::kSignatureLen];
  RINOK(ReadStream_FALSE(inStream, signature, NHeader::kSignatureLen));
  m_Position += NHeader::kSignatureLen;
  if (memcmp(signature, NHeader::kSignature, NHeader::kSignatureLen) != 0)
    return S_FALSE;
  m_Stream = inStream;
  return S_OK;
}

static void MyStrNCpy(char *dest, const char *src, int size)
{
  for (int i = 0; i < size; i++)
  {
    char c = src[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
}

static bool OctalToNumber(const char *s, int size, UInt64 &res)
{
  char sz[32];
  MyStrNCpy(sz, s, size);
  sz[size] = 0;
  const char *end;
  int i;
  for (i = 0; sz[i] == ' '; i++);
  res = ConvertOctStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool OctalToNumber32(const char *s, int size, UInt32 &res)
{
  UInt64 res64;
  if (!OctalToNumber(s, size, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

static bool DecimalToNumber(const char *s, int size, UInt64 &res)
{
  char sz[32];
  MyStrNCpy(sz, s, size);
  sz[size] = 0;
  const char *end;
  int i;
  for (i = 0; sz[i] == ' '; i++);
  res = ConvertStringToUInt64(sz + i, &end);
  return (*end == ' ' || *end == 0);
}

static bool DecimalToNumber32(const char *s, int size, UInt32 &res)
{
  UInt64 res64;
  if (!DecimalToNumber(s, size, res64))
    return false;
  res = (UInt32)res64;
  return (res64 <= 0xFFFFFFFF);
}

#define RIF(x) { if (!(x)) return S_FALSE; }


HRESULT CInArchive::GetNextItemReal(bool &filled, CItemEx &item)
{
  filled = false;

  char header[NHeader::kHeaderSize];
  const char *cur = header;

  size_t processedSize = sizeof(header);
  item.HeaderPosition = m_Position;
  RINOK(ReadStream(m_Stream, header, &processedSize));
  m_Position += processedSize;
  if (processedSize != sizeof(header))
    return S_OK;
  
  char tempString[NHeader::kNameSize + 1];
  MyStrNCpy(tempString, cur, NHeader::kNameSize);
  cur += NHeader::kNameSize;
  tempString[NHeader::kNameSize] = '\0';
  item.Name = tempString;
  item.Name.Trim();

  for (int i = 0; i < item.Name.Length(); i++)
    if (((Byte)item.Name[i]) < 0x20)
      return S_FALSE;

  RIF(DecimalToNumber32(cur, NHeader::kTimeSize, item.MTime));
  cur += NHeader::kTimeSize;

  cur += 6 + 6;
  
  RIF(OctalToNumber32(cur, NHeader::kModeSize, item.Mode));
  cur += NHeader::kModeSize;

  RIF(DecimalToNumber(cur, NHeader::kSizeSize, item.Size));
  cur += NHeader::kSizeSize;

  filled = true;
  return S_OK;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  for (;;)
  {
    RINOK(GetNextItemReal(filled, item));
    if (!filled)
      return S_OK;
    if (item.Name.CompareNoCase("debian-binary") != 0)
      return S_OK;
    if (item.Size != 4)
      return S_OK;
    SkeepData(item.Size);
  }
}

HRESULT CInArchive::Skeep(UInt64 numBytes)
{
  UInt64 newPostion;
  RINOK(m_Stream->Seek(numBytes, STREAM_SEEK_CUR, &newPostion));
  m_Position += numBytes;
  if (m_Position != newPostion)
    return E_FAIL;
  return S_OK;
}

HRESULT CInArchive::SkeepData(UInt64 dataSize)
{
  return Skeep((dataSize + 1) & (~((UInt64)0x1)));
}


class CHandler:
  public IInArchive,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(IInArchive)

  INTERFACE_IInArchive(;)

private:
  CObjectVector<CItemEx> _items;
  CMyComPtr<IInStream> _inStream;
};


STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  {
    CInArchive archive;
    if(archive.Open(stream) != S_OK)
      return S_FALSE;
    _items.Clear();

    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UInt64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

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
      archive.SkeepData(item.Size);
      if (openArchiveCallback != NULL)
      {
        UInt64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
    _inStream = stream;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _inStream.Release();
  _items.Clear();
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
    case kpidPath: prop = (const wchar_t *)NItemName::GetOSName2(MultiByteToUnicodeString(item.Name, CP_OEMCP)); break;
    case kpidSize:
    case kpidPackSize:
      prop = item.Size;
      break;
    case kpidMTime:
    {
      if (item.MTime != 0)
      {
        FILETIME fileTime;
        NTime::UnixTimeToFileTime(item.MTime, fileTime);
        prop = fileTime;
      }
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  bool allFilesMode = (numItems == UInt32(-1));
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
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    currentItemSize = item.Size;
    
    
    
    
    
    
    if (!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    if (testMode)
    {
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    RINOK(_inStream->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
    streamSpec->Init(item.Size);
    RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult((copyCoderSpec->TotalSize == item.Size) ?
        NArchive::NExtract::NOperationResult::kOK:
        NArchive::NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NDeb::CHandler;  }

static CArcInfo g_ArcInfo =
  { L"Deb", L"deb", 0, 0xEC, { '!', '<', 'a', 'r', 'c', 'h', '>', '\n'  }, 8, false, CreateArc, 0 };

REGISTER_ARC(Deb)

}}
