// GzHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/DeflateDecoder.h"
#include "../Compress/DeflateEncoder.h"

#include "Common/InStreamWithCRC.h"
#include "Common/OutStreamWithCRC.h"

#include "DeflateProps.h"

#define Get32(p) GetUi32(p)

using namespace NWindows;

namespace NArchive {
namespace NGz {

static const UInt16 kSignature = 0x8B1F;

namespace NHeader
{
  namespace NFlags
  {
    const Byte kIsText = 1 << 0;
    const Byte kCrc = 1 << 1;
    const Byte kExtra = 1 << 2;
    const Byte kName = 1 << 3;
    const Byte kComment = 1 << 4;
  }
  
  namespace NExtraFlags
  {
    const Byte kMaximum = 2;
    const Byte kFastest = 4;
  }
  
  namespace NCompressionMethod
  {
    const Byte kDeflate = 8;
  }

  namespace NHostOS
  {
    enum EEnum
    {
      kFAT = 0,
      kAMIGA,
      kVMS,
      kUnix,
      kVM_CMS,
      kAtari,
      kHPFS,
      kMac,
      kZ_System,
      kCPM,
      kTOPS20,
      kNTFS,
      kQDOS,
      kAcorn,
      kVFAT,
      kMVS,
      kBeOS,
      kTandem,
      
      kUnknown = 255
    };
  }
}

static const char *kHostOSes[] =
{
  "FAT",
  "AMIGA",
  "VMS",
  "Unix",
  "VM/CMS",
  "Atari",
  "HPFS",
  "Macintosh",
  "Z-System",
  "CP/M",
  "TOPS-20",
  "NTFS",
  "SMS/QDOS",
  "Acorn",
  "VFAT",
  "MVS",
  "BeOS",
  "Tandem",
  "OS/400",
  "OS/X"
};

static const char *kUnknownOS = "Unknown";

class CItem
{
  bool TestFlag(Byte flag) const { return (Flags & flag) != 0; }
public:
  Byte Method;
  Byte Flags;
  Byte ExtraFlags;
  Byte HostOS;
  UInt32 Time;
  UInt32 Crc;
  UInt32 Size32;

  AString Name;
  AString Comment;
  // CByteBuffer Extra;

  // bool IsText() const { return TestFlag(NHeader::NFlags::kIsText); }
  bool HeaderCrcIsPresent() const { return TestFlag(NHeader::NFlags::kCrc); }
  bool ExtraFieldIsPresent() const { return TestFlag(NHeader::NFlags::kExtra); }
  bool NameIsPresent() const { return TestFlag(NHeader::NFlags::kName); }
  bool CommentIsPresent() const { return TestFlag(NHeader::NFlags::kComment); }

  void Clear()
  {
    Name.Empty();
    Comment.Empty();
    // Extra.SetCapacity(0);
  }

  HRESULT ReadHeader(NCompress::NDeflate::NDecoder::CCOMCoder *stream);
  HRESULT ReadFooter1(NCompress::NDeflate::NDecoder::CCOMCoder *stream);
  HRESULT ReadFooter2(ISequentialInStream *stream);

  HRESULT WriteHeader(ISequentialOutStream *stream);
  HRESULT WriteFooter(ISequentialOutStream *stream);
};

static HRESULT ReadBytes(NCompress::NDeflate::NDecoder::CCOMCoder *stream, Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    data[i] = stream->ReadByte();
  return stream->InputEofError() ? S_FALSE : S_OK;
}

static HRESULT SkipBytes(NCompress::NDeflate::NDecoder::CCOMCoder *stream, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    stream->ReadByte();
  return stream->InputEofError() ? S_FALSE : S_OK;
}

static HRESULT ReadUInt16(NCompress::NDeflate::NDecoder::CCOMCoder *stream, UInt16 &value /* , UInt32 &crc */)
{
  value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = stream->ReadByte();
    if (stream->InputEofError())
      return S_FALSE;
    // crc = CRC_UPDATE_BYTE(crc, b);
    value |= (UInt16(b) << (8 * i));
  }
  return S_OK;
}

static HRESULT ReadString(NCompress::NDeflate::NDecoder::CCOMCoder *stream, AString &s, UInt32 limit /* , UInt32 &crc */)
{
  s.Empty();
  for (UInt32 i = 0; i < limit; i++)
  {
    Byte b = stream->ReadByte();
    if (stream->InputEofError())
      return S_FALSE;
    // crc = CRC_UPDATE_BYTE(crc, b);
    if (b == 0)
      return S_OK;
    s += (char)b;
  }
  return S_FALSE;
}

HRESULT CItem::ReadHeader(NCompress::NDeflate::NDecoder::CCOMCoder *stream)
{
  Clear();

  // Header-CRC field had another meaning in old version of gzip!
  // UInt32 crc = CRC_INIT_VAL;
  Byte buf[10];

  RINOK(ReadBytes(stream, buf, 10));
  
  if (GetUi16(buf) != kSignature)
    return S_FALSE;

  Method = buf[2];

  if (Method != NHeader::NCompressionMethod::kDeflate)
    return S_FALSE;

  Flags = buf[3];
  Time = Get32(buf + 4);
  ExtraFlags = buf[8];
  HostOS = buf[9];

  // crc = CrcUpdate(crc, buf, 10);
  
  if (ExtraFieldIsPresent())
  {
    UInt16 extraSize;
    RINOK(ReadUInt16(stream, extraSize /* , crc */));
    RINOK(SkipBytes(stream, extraSize));
    // Extra.SetCapacity(extraSize);
    // RINOK(ReadStream_FALSE(stream, Extra, extraSize));
    // crc = CrcUpdate(crc, Extra, extraSize);
  }
  if (NameIsPresent())
    RINOK(ReadString(stream, Name, (1 << 10) /* , crc */));
  if (CommentIsPresent())
    RINOK(ReadString(stream, Comment, (1 << 16) /* , crc */));

  if (HeaderCrcIsPresent())
  {
    UInt16 headerCRC;
    // UInt32 dummy = 0;
    RINOK(ReadUInt16(stream, headerCRC /* , dummy */));
    /*
    if ((UInt16)CRC_GET_DIGEST(crc) != headerCRC)
      return S_FALSE;
    */
  }
  return stream->InputEofError() ? S_FALSE : S_OK;
}

HRESULT CItem::ReadFooter1(NCompress::NDeflate::NDecoder::CCOMCoder *stream)
{
  Byte buf[8];
  RINOK(ReadBytes(stream, buf, 8));
  Crc = Get32(buf);
  Size32 = Get32(buf + 4);
  return stream->InputEofError() ? S_FALSE : S_OK;
}

HRESULT CItem::ReadFooter2(ISequentialInStream *stream)
{
  Byte buf[8];
  RINOK(ReadStream_FALSE(stream, buf, 8));
  Crc = Get32(buf);
  Size32 = Get32(buf + 4);
  return S_OK;
}

HRESULT CItem::WriteHeader(ISequentialOutStream *stream)
{
  Byte buf[10];
  SetUi16(buf, kSignature);
  buf[2] = Method;
  buf[3] = Flags & NHeader::NFlags::kName;
  // buf[3] |= NHeader::NFlags::kCrc;
  SetUi32(buf + 4, Time);
  buf[8] = ExtraFlags;
  buf[9] = HostOS;
  RINOK(WriteStream(stream, buf, 10));
  // crc = CrcUpdate(CRC_INIT_VAL, buf, 10);
  if (NameIsPresent())
  {
    // crc = CrcUpdate(crc, (const char *)Name, Name.Length() + 1);
    RINOK(WriteStream(stream, (const char *)Name, Name.Length() + 1));
  }
  // SetUi16(buf, (UInt16)CRC_GET_DIGEST(crc));
  // RINOK(WriteStream(stream, buf, 2));
  return S_OK;
}

HRESULT CItem::WriteFooter(ISequentialOutStream *stream)
{
  Byte buf[8];
  SetUi32(buf, Crc);
  SetUi32(buf + 4, Size32);
  return WriteStream(stream, buf, 8);
}

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
  CItem _item;
  UInt64 _startPosition;
  UInt64 _headerSize;
  UInt64 _packSize;
  bool _packSizeDefined;
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ICompressCoder> _decoder;
  NCompress::NDeflate::NDecoder::CCOMCoder *_decoderSpec;

  CDeflateProps _method;

public:
  MY_UNKNOWN_IMP4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)
  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)
  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProps);

  CHandler()
  {
    _decoderSpec = new NCompress::NDeflate::NDecoder::CCOMCoder;
    _decoder = _decoderSpec;
  }
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidCRC, VT_UI4}
  // { NULL, kpidComment, VT_BSTR}
}
;

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize: if (_packSizeDefined) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}


STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPath:
      if (_item.NameIsPresent())
        prop = MultiByteToUnicodeString(_item.Name, CP_ACP);
      break;
    // case kpidComment: if (_item.CommentIsPresent()) prop = MultiByteToUnicodeString(_item.Comment, CP_ACP); break;
    case kpidMTime:
    {
      if (_item.Time != 0)
      {
        FILETIME utc;
        NTime::UnixTimeToFileTime(_item.Time, utc);
        prop = utc;
      }
      break;
    }
    case kpidSize: if (_stream) prop = (UInt64)_item.Size32; break;
    case kpidPackSize: if (_packSizeDefined) prop = _packSize; break;
    case kpidHostOS: prop = (_item.HostOS < sizeof(kHostOSes) / sizeof(kHostOSes[0])) ?
          kHostOSes[_item.HostOS] : kUnknownOS; break;
    case kpidCRC: if (_stream) prop = _item.Crc; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *)
{
  COM_TRY_BEGIN
  HRESULT res;
  try
  {
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_startPosition));
    res = OpenSeq(stream);
    if (res == S_OK)
    {
      UInt64 endPos;
      res = stream->Seek(-8, STREAM_SEEK_END, &endPos);
      _packSize = endPos + 8 - _startPosition;
      _packSizeDefined = true;
      if (res == S_OK)
      {
        res = _item.ReadFooter2(stream);
        _stream = stream;
      }
    }
  }
  catch(...) { res = S_FALSE; }
  if (res != S_OK)
    Close();
  return res;
  COM_TRY_END
}

STDMETHODIMP CHandler::OpenSeq(ISequentialInStream *stream)
{
  COM_TRY_BEGIN
  HRESULT res;
  try
  {
    Close();
    _decoderSpec->SetInStream(stream);
    _decoderSpec->InitInStream(true);
    res = _item.ReadHeader(_decoderSpec);
    _headerSize = _decoderSpec->GetInputProcessedSize();
  }
  catch(...) { res = S_FALSE; }
  if (res != S_OK)
    Close();
  return res;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _packSizeDefined = false;
  _stream.Release();
  _decoderSpec->ReleaseInStream();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  if (_stream)
    extractCallback->SetTotal(_packSize);
  UInt64 currentTotalPacked = 0;
  RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  if (!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);

  if (_stream)
  {
    RINOK(_stream->Seek(_startPosition, STREAM_SEEK_SET, NULL));
    _decoderSpec->InitInStream(true);
  }
  bool firstItem = true;
  Int32 opRes;
  for (;;)
  {
    lps->InSize = _packSize = _decoderSpec->GetInputProcessedSize();
    _packSizeDefined = true;
    lps->OutSize = outStreamSpec->GetSize();
    RINOK(lps->SetCur());

    CItem item;
    if (!firstItem || _stream)
    {
      HRESULT result = item.ReadHeader(_decoderSpec);
      if (result != S_OK)
      {
        if (result != S_FALSE)
          return result;
        opRes = firstItem ?
            NExtract::NOperationResult::kDataError :
            NExtract::NOperationResult::kOK;
        break;
      }
    }
    firstItem = false;

    UInt64 startOffset = outStreamSpec->GetSize();
    outStreamSpec->InitCRC();

    HRESULT result = _decoderSpec->CodeResume(outStream, NULL, progress);
    if (result != S_OK)
    {
      if (result != S_FALSE)
        return result;
      opRes = NExtract::NOperationResult::kDataError;
      break;
    }

    _decoderSpec->AlignToByte();
    if (item.ReadFooter1(_decoderSpec) != S_OK)
    {
      opRes = NExtract::NOperationResult::kDataError;
      break;
    }
    if (item.Crc != outStreamSpec->GetCRC() ||
        item.Size32 != (UInt32)(outStreamSpec->GetSize() - startOffset))
    {
      opRes = NExtract::NOperationResult::kCRCError;
      break;
    }
  }
  outStream.Release();
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

static const Byte kHostOS =
  #ifdef _WIN32
  NHeader::NHostOS::kFAT;
  #else
  NHeader::NHostOS::kUnix;
  #endif

static HRESULT UpdateArchive(
    ISequentialOutStream *outStream,
    UInt64 unpackSize,
    const CItem &newItem,
    CDeflateProps &deflateProps,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;
  RINOK(updateCallback->SetTotal(unpackSize));
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(0, &fileInStream));

  CSequentialInStreamWithCRC *inStreamSpec = new CSequentialInStreamWithCRC;
  CMyComPtr<ISequentialInStream> crcStream(inStreamSpec);
  inStreamSpec->SetStream(fileInStream);
  inStreamSpec->Init();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);
  
  CItem item = newItem;
  item.Method = NHeader::NCompressionMethod::kDeflate;
  item.ExtraFlags = deflateProps.IsMaximum() ?
      NHeader::NExtraFlags::kMaximum :
      NHeader::NExtraFlags::kFastest;

  item.HostOS = kHostOS;

  RINOK(item.WriteHeader(outStream));

  NCompress::NDeflate::NEncoder::CCOMCoder *deflateEncoderSpec = new NCompress::NDeflate::NEncoder::CCOMCoder;
  CMyComPtr<ICompressCoder> deflateEncoder = deflateEncoderSpec;
  RINOK(deflateProps.SetCoderProperties(deflateEncoderSpec));
  RINOK(deflateEncoder->Code(crcStream, outStream, NULL, NULL, progress));

  item.Crc = inStreamSpec->GetCRC();
  item.Size32 = (UInt32)inStreamSpec->GetSize();
  RINOK(item.WriteFooter(outStream));
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *timeType)
{
  *timeType = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (numItems != 1)
    return E_INVALIDARG;

  Int32 newData, newProps;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive));

  CItem newItem = _item;
  newItem.ExtraFlags = 0;
  newItem.Flags = 0;
  if (IntToBool(newProps))
  {
    {
      FILETIME utcTime;
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidMTime, &prop));
      if (prop.vt != VT_FILETIME)
        return E_INVALIDARG;
      utcTime = prop.filetime;
      if (!NTime::FileTimeToUnixTime(utcTime, newItem.Time))
        return E_INVALIDARG;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidPath, &prop));
      if (prop.vt == VT_BSTR)
      {
        UString name = prop.bstrVal;
        int dirDelimiterPos = name.ReverseFind(CHAR_PATH_SEPARATOR);
        if (dirDelimiterPos >= 0)
          name = name.Mid(dirDelimiterPos + 1);
        newItem.Name = UnicodeStringToMultiByte(name, CP_ACP);
        if (!newItem.Name.IsEmpty())
          newItem.Flags |= NHeader::NFlags::kName;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop));
      if (prop.vt == VT_BOOL)
      {
        if (prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
  }

  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }

    return UpdateArchive(outStream, size, newItem, _method, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (!_stream)
    return E_NOTIMPL;

  UInt64 offset = _startPosition;
  if (IntToBool(newProps))
  {
    newItem.WriteHeader(outStream);
    offset += _headerSize;
  }
  RINOK(_stream->Seek(offset, STREAM_SEEK_SET, NULL));
  return NCompress::CopyStream(_stream, outStream, NULL);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProps)
{
  return _method.SetProperties(names, values, numProps);
}

static IInArchive *CreateArc() { return new CHandler; }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new CHandler; }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"gzip", L"gz gzip tgz tpz", L"* * .tar .tar", 0xEF, { 0x1F, 0x8B, 8 }, 3, true, CreateArc, CreateArcOut };

REGISTER_ARC(GZip)

}}
