// ZHandler.cpp

#include "StdAfx.h"

#include "ZHandler.h"

#include "Common/Defs.h"

#include "../../Common/ProgressUtils.h"
#include "../../Compress/Z/ZDecoder.h"
#include "../../Common/StreamUtils.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "Common/ComTry.h"

#include "../Common/DummyOutStream.h"

namespace NArchive {
namespace NZ {

STATPROPSTG kProps[] = 
{
  { NULL, kpidPackedSize, VT_UI8},
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID,  PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPackedSize: prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

static const int kSignatureSize = 3;

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  try
  {
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition));
    Byte buffer[kSignatureSize];
    UInt32 processedSize;
    RINOK(ReadStream(stream, buffer, kSignatureSize, &processedSize));
    if (processedSize != kSignatureSize)
      return S_FALSE;
    if (buffer[0] != 0x1F || buffer[1] != 0x9D)
      return S_FALSE;
    _properties = buffer[2];

    UInt64 endPosition;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPosition));
    _packSize = endPosition - _streamStartPosition - kSignatureSize;
    
    _stream = stream;
  }
  catch(...)
  {
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)(-1));
  if (!allFilesMode)
  {
    if (numItems == 0)
      return S_OK;
    if (numItems != 1)
      return E_INVALIDARG;
    if (indices[0] != 0)
      return E_INVALIDARG;
  }

  bool testMode = (testModeSpec != 0);

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

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);
  
  RINOK(_stream->Seek(_streamStartPosition + kSignatureSize, STREAM_SEEK_SET, NULL));

  CMyComPtr<ICompressCoder> decoder;
  NCompress::NZ::CDecoder *decoderSpec = new NCompress::NZ::CDecoder;
  decoder = decoderSpec;

  HRESULT result = decoderSpec->SetDecoderProperties2(&_properties, 1);

  int opResult;
  if (result != S_OK)
    opResult = NExtract::NOperationResult::kUnSupportedMethod;
  else
  {
    result = decoder->Code(_stream, outStream, NULL, NULL, progress);
    if (result == S_FALSE)
      opResult = NExtract::NOperationResult::kDataError;
    else
    {
      RINOK(result);
      opResult = NExtract::NOperationResult::kOK;
    }
  }
  outStream.Release();
  return extractCallback->SetOperationResult(opResult);
  COM_TRY_END
}

}}
