// MubHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"
#include "../../Common/IntToString.h"
#include "../../Common/MyString.h"

#include "../../Windows/PropVariant.h"

#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"

static UInt32 Get32(const Byte *p, bool be) { if (be) return GetBe32(p); return GetUi32(p); }

using namespace NWindows;
using namespace NCOM;

namespace NArchive {
namespace NMub {

#define MACH_CPU_ARCH_ABI64 (1 << 24)
#define MACH_CPU_TYPE_386    7
#define MACH_CPU_TYPE_ARM   12
#define MACH_CPU_TYPE_SPARC 14
#define MACH_CPU_TYPE_PPC   18

#define MACH_CPU_TYPE_PPC64 (MACH_CPU_ARCH_ABI64 | MACH_CPU_TYPE_PPC)
#define MACH_CPU_TYPE_AMD64 (MACH_CPU_ARCH_ABI64 | MACH_CPU_TYPE_386)

#define MACH_CPU_SUBTYPE_LIB64 (1 << 31)

#define	MACH_CPU_SUBTYPE_I386_ALL	3

struct CItem
{
  UInt32 Type;
  UInt32 SubType;
  UInt32 Offset;
  UInt32 Size;
  // UInt32 Align;
};

static const UInt32 kNumFilesMax = 10;

class CHandler:
  public IInArchive,
  public IInArchiveGetStream,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  // UInt64 _startPos;
  UInt64 _phySize;
  UInt32 _numItems;
  bool _bigEndian;
  CItem _items[kNumFilesMax];

  HRESULT Open2(IInStream *stream);
public:
  MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
  INTERFACE_IInArchive(;)
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **stream);
};

static const Byte kArcProps[] =
{
  kpidBigEndian
};

static const Byte kProps[] =
{
  kpidSize
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  PropVariant_Clear(value);
  switch (propID)
  {
    case kpidBigEndian: PropVarEm_Set_Bool(value, _bigEndian); break;
    case kpidPhySize: PropVarEm_Set_UInt64(value, _phySize); break;
  }
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  PropVariant_Clear(value);
  const CItem &item = _items[index];
  switch (propID)
  {
    case kpidExtension:
    {
      char temp[32];
      const char *ext = 0;
      switch (item.Type)
      {
        case MACH_CPU_TYPE_386:   ext = "x86";    break;
        case MACH_CPU_TYPE_ARM:   ext = "arm";   break;
        case MACH_CPU_TYPE_SPARC: ext = "sparc"; break;
        case MACH_CPU_TYPE_PPC:   ext = "ppc";   break;
        case MACH_CPU_TYPE_PPC64: ext = "ppc64"; break;
        case MACH_CPU_TYPE_AMD64: ext = "x64";   break;
        default:
          temp[0] = 'c';
          temp[1] = 'p';
          temp[2] = 'u';
          ConvertUInt32ToString(item.Type, temp + 3);
          break;
      }
      if (ext)
        strcpy(temp, ext);
      if (item.SubType != 0 && (
          item.Type != MACH_CPU_TYPE_386 &&
          item.Type != MACH_CPU_TYPE_AMD64 ||
          (item.SubType & ~(UInt32)MACH_CPU_SUBTYPE_LIB64) != MACH_CPU_SUBTYPE_I386_ALL))
      {
        unsigned pos = MyStringLen(temp);
        temp[pos++] = '-';
          ConvertUInt32ToString(item.SubType, temp + pos);
      }
      return PropVarEm_Set_Str(value, temp);
    }
    case kpidSize:
    case kpidPackSize:
      PropVarEm_Set_UInt64(value, item.Size);
      break;
  }
  return S_OK;
}

HRESULT CHandler::Open2(IInStream *stream)
{
  // RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_startPos));

  const UInt32 kHeaderSize = 8;
  const UInt32 kRecordSize = 5 * 4;
  const UInt32 kBufSize = kHeaderSize + kNumFilesMax * kRecordSize;
  Byte buf[kBufSize];
  size_t processed = kBufSize;
  RINOK(ReadStream(stream, buf, &processed));
  if (processed < kHeaderSize)
    return S_FALSE;
  
  bool be;
  switch (GetBe32(buf))
  {
    case 0xCAFEBABE: be = true; break;
    case 0xB9FAF10E: be = false; break;
    default: return S_FALSE;
  }
  _bigEndian = be;
  UInt32 num = Get32(buf + 4, be);
  if (num > kNumFilesMax || processed < kHeaderSize + num * kRecordSize)
    return S_FALSE;
  if (num == 0)
    return S_FALSE;
  UInt64 endPosMax = kHeaderSize;

  for (UInt32 i = 0; i < num; i++)
  {
    const Byte *p = buf + kHeaderSize + i * kRecordSize;
    CItem &sb = _items[i];
    sb.Type = Get32(p, be);
    sb.SubType = Get32(p + 4, be);
    sb.Offset = Get32(p + 8, be);
    sb.Size = Get32(p + 12, be);
    UInt32 align = Get32(p + 16, be);
    if (align > 31)
      return S_FALSE;
    if (sb.Offset < kHeaderSize + num * kRecordSize)
      return S_FALSE;
    if ((sb.Type & ~MACH_CPU_ARCH_ABI64) >= 0x100 ||
        (sb.SubType & ~MACH_CPU_SUBTYPE_LIB64) >= 0x100)
      return S_FALSE;

    UInt64 endPos = (UInt64)sb.Offset + sb.Size;
    if (endPosMax < endPos)
      endPosMax = endPos;
  }
  _numItems = num;
  _phySize = endPosMax;
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
  _phySize = 0;
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
  bool allFilesMode = (numItems == (UInt32)(Int32)-1);
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
    RINOK(_stream->Seek(/* _startPos + */ item.Offset, STREAM_SEEK_SET, NULL));
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
  return CreateLimitedInStream(_stream, /* _startPos + */ item.Offset, item.Size, stream);
  COM_TRY_END
}

IMP_CreateArcIn

namespace NBe {

static CArcInfo g_ArcInfo =
  { "Mub", "mub", 0, 0xE2,
  2 + 7 + 4,
  {
    7, 0xCA, 0xFE, 0xBA, 0xBE, 0, 0, 0,
    4, 0xB9, 0xFA, 0xF1, 0x0E
  },
  0,
  NArcInfoFlags::kMultiSignature,
  CreateArc };

REGISTER_ARC(Mub)

}

}}
