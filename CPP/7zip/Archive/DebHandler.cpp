// DebHandler.cpp

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

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NDeb {

namespace NHeader
{
  const int kSignatureLen = 8;
  
  const char *kSignature = "!<arch>\n";

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

struct CItem
{
  AString Name;
  UInt64 Size;
  UInt32 MTime;
  UInt32 Mode;

  UInt64 HeaderPos;
  UInt64 GetDataPos() const { return HeaderPos + NHeader::kHeaderSize; };
  // UInt64 GetFullSize() const { return NFileHeader::kRecordSize + Size; };
};

class CInArchive
{
  CMyComPtr<IInStream> m_Stream;
  
  HRESULT GetNextItemReal(bool &filled, CItem &itemInfo);
public:
  UInt64 m_Position;
  HRESULT Open(IInStream *inStream);
  HRESULT GetNextItem(bool &filled, CItem &itemInfo);
  HRESULT SkipData(UInt64 dataSize);
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


HRESULT CInArchive::GetNextItemReal(bool &filled, CItem &item)
{
  filled = false;

  char header[NHeader::kHeaderSize];
  const char *cur = header;

  size_t processedSize = sizeof(header);
  item.HeaderPos = m_Position;
  RINOK(ReadStream(m_Stream, header, &processedSize));
  if (processedSize != sizeof(header))
    return S_OK;
  m_Position += processedSize;
  
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

HRESULT CInArchive::GetNextItem(bool &filled, CItem &item)
{
  for (;;)
  {
    RINOK(GetNextItemReal(filled, item));
    if (!filled)
      return S_OK;
    if (item.Name.Compare("debian-binary") != 0)
      return S_OK;
    if (item.Size != 4)
      return S_OK;
    SkipData(item.Size);
  }
}

HRESULT CInArchive::SkipData(UInt64 dataSize)
{
  return m_Stream->Seek((dataSize + 1) & (~((UInt64)0x1)), STREAM_SEEK_CUR, &m_Position);
}

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CObjectVector<CItem> _items;
  CMyComPtr<IInStream> _stream;
  Int32 _mainSubfile;
  UInt64 _phySize;
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

static STATPROPSTG kArcProps[] =
{
  { NULL, kpidPhySize, VT_UI8}
};

static STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  {
    _mainSubfile = -1;
    CInArchive archive;
    if (archive.Open(stream) != S_OK)
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
      CItem item;
      bool filled;
      HRESULT result = archive.GetNextItem(filled, item);
      if (result == S_FALSE)
        return S_FALSE;
      if (result != S_OK)
        return S_FALSE;
      if (!filled)
        break;
      if (item.Name.Left(5) == "data.")
        _mainSubfile = _items.Size();
      _items.Add(item);
      archive.SkipData(item.Size);
      if (openArchiveCallback != NULL)
      {
        UInt64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
    _stream = stream;
    _phySize = archive.m_Position;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _items.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize: prop = _phySize; break;
    case kpidMainSubfile: if (_mainSubfile >= 0) prop = (UInt32)_mainSubfile; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _items[index];

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
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    currentTotalSize += item.Size;
    
    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    if (testMode)
    {
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    RINOK(_stream->Seek(item.GetDataPos(), STREAM_SEEK_SET, NULL));
    streamSpec->Init(item.Size);
    RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
    realOutStream.Release();
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
  const CItem &item = _items[index];
  return CreateLimitedInStream(_stream, item.GetDataPos(), item.Size, stream);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new NArchive::NDeb::CHandler; }

static CArcInfo g_ArcInfo =
  { L"Deb", L"deb", 0, 0xEC, { '!', '<', 'a', 'r', 'c', 'h', '>', '\n' }, 8, false, CreateArc, 0 };

REGISTER_ARC(Deb)

}}
