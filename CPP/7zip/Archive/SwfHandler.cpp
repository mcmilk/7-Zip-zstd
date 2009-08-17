// SwfHandler.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "Common/Buffer.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/MyString.h"

#include "Windows/PropVariant.h"

#include "../Common/InBuffer.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"
#include "../Compress/ZlibEncoder.h"

#include "Common/DummyOutStream.h"

#include "DeflateProps.h"

using namespace NWindows;

namespace NArchive {
namespace NSwfc {

static const UInt32 kHeaderSize = 8;

static const Byte SWF_UNCOMPRESSED = 'F';
static const Byte SWF_COMPRESSED = 'C';
static const Byte SWF_MIN_COMPRESSED_VER = 6;

struct CItem
{
  Byte Buf[kHeaderSize];

  UInt32 GetSize() const { return GetUi32(Buf + 4); }
  bool IsSwf(Byte c) const { return (Buf[0] == c && Buf[1] == 'W' && Buf[2] == 'S' && Buf[3] < 32); }
  bool IsUncompressed() const { return IsSwf(SWF_UNCOMPRESSED); }
  bool IsCompressed() const { return IsSwf(SWF_COMPRESSED); }

  void MakeUncompressed() { Buf[0] = SWF_UNCOMPRESSED; }
  void MakeCompressed()
  {
    Buf[0] = SWF_COMPRESSED;
    if (Buf[3] < SWF_MIN_COMPRESSED_VER)
      Buf[3] = SWF_MIN_COMPRESSED_VER;
  }

  HRESULT ReadHeader(ISequentialInStream *stream) { return ReadStream_FALSE(stream, Buf, kHeaderSize); }
  HRESULT WriteHeader(ISequentialOutStream *stream) { return WriteStream(stream, Buf, kHeaderSize); }
};

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
  CItem _item;
  UInt64 _packSize;
  bool _packSizeDefined;
  CMyComPtr<ISequentialInStream> _seqStream;
  CMyComPtr<IInStream> _stream;

  CDeflateProps _method;

public:
  MY_UNKNOWN_IMP4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)
  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)
  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProps);
};

STATPROPSTG kProps[] =
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8}
};

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

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSize: prop = (UInt64)_item.GetSize(); break;
    case kpidPackSize: if (_packSizeDefined) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *)
{
  RINOK(OpenSeq(stream));
  _stream = stream;
  return S_OK;
}

STDMETHODIMP CHandler::OpenSeq(ISequentialInStream *stream)
{
  Close();
  HRESULT res = _item.ReadHeader(stream);
  if (res == S_OK)
    if (_item.IsCompressed())
      _seqStream = stream;
    else
      res = S_FALSE;
  return res;
}

STDMETHODIMP CHandler::Close()
{
  _packSizeDefined = false;
  _seqStream.Release();
  _stream.Release();
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

  extractCallback->SetTotal(_item.GetSize());
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  if (!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  NCompress::NZlib::CDecoder *_decoderSpec = new NCompress::NZlib::CDecoder;
  CMyComPtr<ICompressCoder> _decoder = _decoderSpec;

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  lps->InSize = kHeaderSize;
  lps->OutSize = outStreamSpec->GetSize();
  RINOK(lps->SetCur());
  
  CItem item = _item;
  item.MakeUncompressed();
  RINOK(item.WriteHeader(outStream));
  if (_stream)
    RINOK(_stream->Seek(kHeaderSize, STREAM_SEEK_SET, NULL));
  HRESULT result = _decoderSpec->Code(_seqStream, outStream, NULL, NULL, progress);
  Int32 opRes = NExtract::NOperationResult::kDataError;
  if (result == S_OK)
  {
    if (_item.GetSize() == outStreamSpec->GetSize())
    {
      _packSizeDefined = true;
      _packSize = _decoderSpec->GetInputProcessedSize() + kHeaderSize;
      opRes = NExtract::NOperationResult::kOK;
    }
  }
  else if (result != S_FALSE)
    return result;

  outStream.Release();
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

static HRESULT UpdateArchive(ISequentialOutStream *outStream,
    UInt64 size, CDeflateProps &deflateProps,
    IArchiveUpdateCallback *updateCallback)
{
  UInt64 complexity = 0;
  RINOK(updateCallback->SetTotal(size));
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;
  RINOK(updateCallback->GetStream(0, &fileInStream));

  CItem item;
  HRESULT res = item.ReadHeader(fileInStream);
  if (res == S_FALSE)
    return E_INVALIDARG;
  RINOK(res);
  if (!item.IsUncompressed() || size != item.GetSize())
    return E_INVALIDARG;

  item.MakeCompressed();
  item.WriteHeader(outStream);

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(updateCallback, true);
  
  NCompress::NZlib::CEncoder *encoderSpec = new NCompress::NZlib::CEncoder;
  CMyComPtr<ICompressCoder> encoder = encoderSpec;
  encoderSpec->Create();
  RINOK(deflateProps.SetCoderProperties(encoderSpec->DeflateEncoderSpec));
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, progress));
  if (encoderSpec->GetInputProcessedSize() + kHeaderSize != size)
    return E_INVALIDARG;
  return updateCallback->SetOperationResult(NUpdate::NOperationResult::kOK);
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

  if (IntToBool(newProps))
  {
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
    return UpdateArchive(outStream, size, _method, updateCallback);
  }
    
  if (indexInArchive != 0)
    return E_INVALIDARG;

  if (!_seqStream)
    return E_NOTIMPL;

  if (_stream)
  {
    RINOK(_stream->Seek(0, STREAM_SEEK_SET, NULL));
  }
  else
    _item.WriteHeader(outStream);
  return NCompress::CopyStream(_seqStream, outStream, NULL);
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
  { L"SWFc", L"swf", L"~.swf", 0xD8, { 'C', 'W', 'S' }, 3, true, CreateArc, CreateArcOut };

REGISTER_ARC(Swfc)

}

namespace NSwf {

static const UInt32 kFileSizeMax = (UInt32)1 << 30;
static const int kNumTagsMax = (UInt32)1 << 23;

struct CTag
{
  UInt32 Type;
  CByteBuffer Buf;
};

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public CMyUnknownImp
{
  CObjectVector<CTag> _tags;
  NSwfc::CItem _item;
  UInt64 _packSize;

  HRESULT OpenSeq3(ISequentialInStream *stream, IArchiveOpenCallback *callback);
  HRESULT OpenSeq2(ISequentialInStream *stream, IArchiveOpenCallback *callback);
public:
  MY_UNKNOWN_IMP2(IInArchive, IArchiveOpenSeq)
  INTERFACE_IInArchive(;)

  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidComment, VT_BSTR}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize: prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}


STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _tags.Size();
  return S_OK;
}

static const char *g_TagDesc[92] =
{
  "End",
  "ShowFrame",
  "DefineShape",
  NULL,
  "PlaceObject",
  "RemoveObject",
  "DefineBits",
  "DefineButton",
  "JPEGTables",
  "SetBackgroundColor",
  "DefineFont",
  "DefineText",
  "DoAction",
  "DefineFontInfo",
  "DefineSound",
  "StartSound",
  NULL,
  "DefineButtonSound",
  "SoundStreamHead",
  "SoundStreamBlock",
  "DefineBitsLossless",
  "DefineBitsJPEG2",
  "DefineShape2",
  "DefineButtonCxform",
  "Protect",
  NULL,
  "PlaceObject2",
  NULL,
  "RemoveObject2",
  NULL,
  NULL,
  NULL,
  "DefineShape3",
  "DefineText2",
  "DefineButton2",
  "DefineBitsJPEG3",
  "DefineBitsLossless2",
  "DefineEditText",
  NULL,
  "DefineSprite",
  NULL,
  "41",
  NULL,
  "FrameLabel",
  NULL,
  "SoundStreamHead2",
  "DefineMorphShape",
  NULL,
  "DefineFont2",
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  "ExportAssets",
  "ImportAssets",
  "EnableDebugger",
  "DoInitAction",
  "DefineVideoStream",
  "VideoFrame",
  "DefineFontInfo2",
  NULL,
  "EnableDebugger2",
  "ScriptLimits",
  "SetTabIndex",
  NULL,
  NULL,
  "FileAttributes",
  "PlaceObject3",
  "ImportAssets2",
  NULL,
  "DefineFontAlignZones",
  "CSMTextSettings",
  "DefineFont3",
  "SymbolClass",
  "Metadata",
  "DefineScalingGrid",
  NULL,
  NULL,
  NULL,
  "DoABC",
  "DefineShape4",
  "DefineMorphShape2",
  NULL,
  "DefineSceneAndFrameLabelData",
  "DefineBinaryData",
  "DefineFontName",
  "StartSound2",
  "DefineBitsJPEG4",
  "DefineFont4"
};

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  const CTag &tag = _tags[index];
  switch(propID)
  {
    case kpidPath:
    {
      char s[32];
      ConvertUInt32ToString(index, s);
      size_t i = strlen(s);
      s[i++] = '.';
      ConvertUInt32ToString(tag.Type, s + i);
      prop = s;
      break;
    }
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)tag.Buf.GetCapacity(); break;
    case kpidComment:
      if (tag.Type < sizeof(g_TagDesc) / sizeof(g_TagDesc[0]))
      {
        const char *s = g_TagDesc[tag.Type];
        if (s != NULL)
          prop = s;
      }
      break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)
{
  return OpenSeq2(stream, callback);
}

static UInt16 Read16(CInBuffer &stream)
{
  UInt16 res = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b;
    if (!stream.ReadByte(b))
      throw 1;
    res |= (UInt16)b << (i * 8);
  }
  return res;
}

static UInt32 Read32(CInBuffer &stream)
{
  UInt32 res = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b;
    if (!stream.ReadByte(b))
      throw 1;
    res |= (UInt32)b << (i * 8);
  }
  return res;
}

struct CBitReader
{
  CInBuffer *stream;
  unsigned NumBits;
  Byte Val;

  CBitReader(): NumBits(0), Val(0) {}

  UInt32 ReadBits(unsigned numBits);
};

UInt32 CBitReader::ReadBits(unsigned numBits)
{
  UInt32 res = 0;
  while (numBits > 0)
  {
    if (NumBits == 0)
    {
      Val = stream->ReadByte();
      NumBits = 8;
    }
    if (numBits <= NumBits)
    {
      res <<= numBits;
      NumBits -= numBits;
      res |= (Val >> NumBits);
      Val &= (1 << NumBits) - 1;
      break;
    }
    else
    {
      res <<= NumBits;
      res |= Val;
      numBits -= NumBits;
      NumBits = 0;
    }
  }
  return res;
}

HRESULT CHandler::OpenSeq3(ISequentialInStream *stream, IArchiveOpenCallback *callback)
{
  RINOK(_item.ReadHeader(stream))
  if (!_item.IsUncompressed())
    return S_FALSE;
  
  CInBuffer s;
  if (!s.Create(1 << 20))
    return E_OUTOFMEMORY;
  s.SetStream(stream);
  s.Init();
  {
    CBitReader br;
    br.stream = &s;
    unsigned numBits = br.ReadBits(5);
    /* UInt32 xMin = */ br.ReadBits(numBits);
    /* UInt32 xMax = */ br.ReadBits(numBits);
    /* UInt32 yMin = */ br.ReadBits(numBits);
    /* UInt32 yMax = */ br.ReadBits(numBits);
  }
  /* UInt32 frameDelay = */ Read16(s);
  /* UInt32 numFrames =  */ Read16(s);

  _tags.Clear();
  UInt64 offsetPrev = 0;
  for (;;)
  {
    UInt32 pair = Read16(s);
    UInt32 type = pair >> 6;
    UInt32 length = pair & 0x3F;
    if (length == 0x3F)
      length = Read32(s);
    if (type == 0)
      break;
    UInt64 offset = s.GetProcessedSize() + NSwfc::kHeaderSize + length;
    if (offset > kFileSizeMax || _tags.Size() >= kNumTagsMax)
      return S_FALSE;
    _tags.Add(CTag());
    CTag &tag = _tags.Back();
    tag.Type = type;
    tag.Buf.SetCapacity(length);
    if (s.ReadBytes(tag.Buf, length) != length)
      return S_FALSE;
    if (callback && offset >= offsetPrev + (1 << 20))
    {
      UInt64 numItems = _tags.Size();
      RINOK(callback->SetCompleted(&numItems, &offset));
      offsetPrev = offset;
    }
  }
  _packSize = s.GetProcessedSize() + NSwfc::kHeaderSize;
  return S_OK;
}

HRESULT CHandler::OpenSeq2(ISequentialInStream *stream, IArchiveOpenCallback *callback)
{
  HRESULT res;
  try { res = OpenSeq3(stream, callback); }
  catch(...) { res = S_FALSE; }
  return res;
}

STDMETHODIMP CHandler::OpenSeq(ISequentialInStream *stream)
{
  return OpenSeq2(stream, NULL);
}

STDMETHODIMP CHandler::Close()
{
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _tags.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _tags[allFilesMode ? i : indices[i]].Buf.GetCapacity();
  extractCallback->SetTotal(totalSize);

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  totalSize = 0;

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    const CByteBuffer &buf = _tags[index].Buf;
    totalSize += buf.GetCapacity();

    CMyComPtr<ISequentialOutStream> outStream;
    RINOK(extractCallback->GetStream(index, &outStream, askMode));
    if (!testMode && !outStream)
      continue;
      
    RINOK(extractCallback->PrepareOperation(askMode));
    if (outStream)
      RINOK(WriteStream(outStream, buf, buf.GetCapacity()));
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
  }
  return S_OK;
  COM_TRY_END
}

static IInArchive *CreateArc() { return new CHandler; }

static CArcInfo g_ArcInfo =
  { L"SWF", L"swf", 0, 0xD7, { 'F', 'W', 'S' }, 3, true, CreateArc, 0 };

REGISTER_ARC(Swf)

}}
