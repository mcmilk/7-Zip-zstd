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

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidPackedSize, VT_UI8},
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if(index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &srcItem = kProperties[index];
  *propID = srcItem.propid;
  *varType = srcItem.vt;
  *name = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UInt32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_INVALIDARG;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  if (index != 0)
    return E_INVALIDARG;
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidPackedSize:
      propVariant = _packSize;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

static const int kSignatureSize = 3;

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
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
  Int32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
  NArchive::NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
    
  if(!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->Init(realOutStream);
  
  realOutStream.Release();

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, true);
  
  RINOK(_stream->Seek(_streamStartPosition + kSignatureSize, STREAM_SEEK_SET, NULL));

  CMyComPtr<ICompressCoder> decoder;
  NCompress::NZ::CDecoder *decoderSpec = new NCompress::NZ::CDecoder;
  decoder = decoderSpec;

  HRESULT result = decoderSpec->SetDecoderProperties2(&_properties, 1);

  int opResult;
  if (result != S_OK)
    opResult = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
  else
  {
    result = decoder->Code(_stream, outStream, NULL, NULL, progress);
    outStream.Release();
    if (result == S_FALSE)
      opResult = NArchive::NExtract::NOperationResult::kDataError;
    else if (result == S_OK)
      opResult = NArchive::NExtract::NOperationResult::kOK;
    else
      return result;
  }
  RINOK(extractCallback->SetOperationResult(opResult));
  return S_OK;
  COM_TRY_END
}

}}
