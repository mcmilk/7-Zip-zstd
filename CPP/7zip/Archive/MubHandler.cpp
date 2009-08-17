// MubHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"

#include "Windows/PropVariant.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

#define Get32(p) GetBe32(p)

namespace NArchive {
namespace NMub {

struct CItem
{
  UInt32 Type;
  UInt32 SubType;
  UInt64 Offset;
  UInt64 Size;
  UInt32 Align;
  bool IsTail;
};

const UInt32 kNumFilesMax = 10;

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  UInt64 _startPos;
  CMyComPtr<IInStream> _stream;
  UInt32 _numItems;
  CItem _items[kNumFilesMax + 1];
  HRESULT Open2(IInStream *stream);
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

#define MACH_ARCH_ABI64  0x1000000
#define MACH_MACHINE_386   7
#define MACH_MACHINE_ARM   12
#define MACH_MACHINE_SPARC 14
#define MACH_MACHINE_PPC   18

#define MACH_MACHINE_PPC64 (MACH_MACHINE_PPC | MACH_ARCH_ABI64)
#define MACH_MACHINE_AMD64 (MACH_MACHINE_386 | MACH_ARCH_ABI64)

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _items[index];
  switch(propID)
  {
    case kpidExtension:
    {
      const wchar_t *ext;
      if (item.IsTail)
        ext = L"tail";
      else
      {
        switch(item.Type)
        {
          case MACH_MACHINE_386:   ext = L"86";    break;
          case MACH_MACHINE_ARM:   ext = L"arm";   break;
          case MACH_MACHINE_SPARC: ext = L"sparc"; break;
          case MACH_MACHINE_PPC:   ext = L"ppc";   break;
          case MACH_MACHINE_PPC64: ext = L"ppc64"; break;
          case MACH_MACHINE_AMD64: ext = L"x64";   break;
          default: ext = L"unknown"; break;
        }
      }
      prop = ext;
      break;
    }
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)item.Size;
      break;
  }
  prop.Detach(value);
  return S_OK;
}

#define MACH_TYPE_ABI64 (1 << 24)
#define MACH_SUBTYPE_ABI64 (1 << 31)

HRESULT CHandler::Open2(IInStream *stream)
{
  RINOK(stream->Seek(0, STREAM_SEEK_SET, &_startPos));

  const UInt32 kHeaderSize = 8;
  const UInt32 kRecordSize = 5 * 4;
  const UInt32 kBufSize = kHeaderSize + kNumFilesMax * kRecordSize;
  Byte buf[kBufSize];
  size_t processed = kBufSize;
  RINOK(ReadStream(stream, buf, &processed));
  if (processed < kHeaderSize)
    return S_FALSE;
  UInt32 num = Get32(buf + 4);
  if (Get32(buf) != 0xCAFEBABE || num > kNumFilesMax || processed < kHeaderSize + num * kRecordSize)
    return S_FALSE;
  UInt64 endPosMax = kHeaderSize;
  for (UInt32 i = 0; i < num; i++)
  {
    const Byte *p = buf + kHeaderSize + i * kRecordSize;
    CItem &sb = _items[i];
    sb.IsTail = false;
    sb.Type = Get32(p);
    sb.SubType = Get32(p + 4);
    sb.Offset = Get32(p + 8);
    sb.Size = Get32(p + 12);
    sb.Align = Get32(p + 16);

    if ((sb.Type & ~MACH_TYPE_ABI64) >= 0x100 ||
        (sb.SubType & ~MACH_SUBTYPE_ABI64) >= 0x100 ||
        sb.Align > 31)
      return S_FALSE;

    UInt64 endPos = (UInt64)sb.Offset + sb.Size;
    if (endPos > endPosMax)
      endPosMax = endPos;
  }
  UInt64 fileSize;
  RINOK(stream->Seek(0, STREAM_SEEK_END, &fileSize));
  fileSize -= _startPos;
  _numItems = num;
  if (fileSize > endPosMax)
  {
    CItem &sb = _items[_numItems++];
    sb.IsTail = true;
    sb.Type = 0;
    sb.SubType = 0;
    sb.Offset = endPosMax;
    sb.Size = fileSize - endPosMax;
    sb.Align = 0;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    if (Open2(inStream) != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _numItems = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _numItems;
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _numItems;
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
    UInt32 index = allFilesMode ? i : indices[i];
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
    RINOK(_stream->Seek(_startPos + item.Offset, STREAM_SEEK_SET, NULL));
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
  return CreateLimitedInStream(_stream, _startPos + item.Offset, item.Size, stream);
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"Mub", L"", 0, 0xE2, { 0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0 }, 7, false, CreateArc, 0 };

REGISTER_ARC(Mub)

}}
