// Bz2Handler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/BZip2Decoder.h"
#include "../Compress/BZip2Encoder.h"
#include "../Compress/CopyCoder.h"

#include "Common/DummyOutStream.h"
#include "Common/HandlerOut.h"

using namespace NWindows;

namespace NArchive {
namespace NBz2 {

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  UInt64 _packSize;
  UInt64 _startPosition;
  bool _packSizeDefined;

  CSingleMethodProps _props;

public:
  MY_UNKNOWN_IMP4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)
  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProps);

  CHandler() { }
};

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPackSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch (propID)
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
  NWindows::NCOM::CPropVariant prop;
  switch (propID)
  {
    case kpidPackSize: if (_packSizeDefined) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  try
  {
    Close();
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_startPosition));
    const int kSignatureSize = 3;
    Byte buf[kSignatureSize];
    RINOK(ReadStream_FALSE(stream, buf, kSignatureSize));
    if (buf[0] != 'B' || buf[1] != 'Z' || buf[2] != 'h')
      return S_FALSE;

    UInt64 endPosition;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPosition));
    _packSize = endPosition - _startPosition;
    _packSizeDefined = true;
    _stream = stream;
    _seqStream = stream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::OpenSeq(ISequentialInStream *stream)
{
  Close();
  _seqStream = stream;
  return S_OK;
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

  NCompress::NBZip2::CDecoder *decoderSpec = new NCompress::NBZip2::CDecoder;
  CMyComPtr<ICompressCoder> decoder = decoderSpec;

  if (_stream)
  {
    RINOK(_stream->Seek(_startPosition, STREAM_SEEK_SET, NULL));
  }

  decoderSpec->SetInStream(_seqStream);

  #ifndef _7ZIP_ST
  RINOK(decoderSpec->SetNumberOfThreads(_props._numThreads));
  #endif

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);

  HRESULT result = S_OK;

  bool firstItem = true;
  for (;;)
  {
    lps->InSize = currentTotalPacked;
    lps->OutSize = outStreamSpec->GetSize();

    RINOK(lps->SetCur());

    bool isBz2;
    result = decoderSpec->CodeResume(outStream, isBz2, progress);

    if (result != S_OK)
      break;
    if (!isBz2)
    {
      if (firstItem)
        result = S_FALSE;
      break;
    }
    firstItem = false;

    _packSize = currentTotalPacked = decoderSpec->GetInputProcessedSize();
    _packSizeDefined = true;
  }
  decoderSpec->ReleaseInStream();
  outStream.Release();

  Int32 retResult;
  if (result == S_OK)
    retResult = NExtract::NOperationResult::kOK;
  else if (result == S_FALSE)
    retResult = NExtract::NOperationResult::kDataError;
  else
    return result;
  return extractCallback->SetOperationResult(retResult);

  COM_TRY_END
}

static HRESULT UpdateArchive(
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    const CProps &props,
    IArchiveUpdateCallback *updateCallback)
{
  RINOK(updateCallback->SetTotal(unpackSize));
  CMyComPtr<ISequentialInStream> fileInStream;
  RINOK(updateCallback->GetStream(0, &fileInStream));
  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  NCompress::NBZip2::CEncoder *encoderSpec = new NCompress::NBZip2::CEncoder;
  CMyComPtr<ICompressCoder> encoder = encoderSpec;
  RINOK(props.SetCoderProps(encoderSpec, NULL));
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
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
    return UpdateArchive(size, outStream, _props, updateCallback);
  }
  if (indexInArchive != 0)
    return E_INVALIDARG;
  if (_stream)
    RINOK(_stream->Seek(_startPosition, STREAM_SEEK_SET, NULL));
  return NCompress::CopyStream(_stream, outStream, NULL);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProps)
{
  return _props.SetProperties(names, values, numProps);
}

static IInArchive *CreateArc() { return new CHandler; }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new CHandler; }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"bzip2", L"bz2 bzip2 tbz2 tbz", L"* * .tar .tar", 2, { 'B', 'Z', 'h' }, 3, true, CreateArc, CreateArcOut };

REGISTER_ARC(BZip2)

}}
