// BZip2Handler.cpp

#include "StdAfx.h"

#include "BZip2Handler.h"

#include "Common/Defs.h"

#include "../../Common/ProgressUtils.h"
// #include "Interface/EnumStatProp.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "Common/ComTry.h"

#include "../Common/DummyOutStream.h"

#ifdef COMPRESS_BZIP2
#include "../../Compress/BZip2/BZip2Decoder.h"
#else
// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
#include "../Common/CoderLoader.h"
extern CSysString GetBZip2CodecPath();
#endif

using namespace NWindows;

namespace NArchive {
namespace NBZip2 {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  // { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UINT32 index,     
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

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_INVALIDARG;
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
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
      propVariant = _item.PackSize;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition));
    const kSignatureSize = 3;
    BYTE buffer[kSignatureSize];
    UINT32 processedSize;
    RINOK(stream->Read(buffer, kSignatureSize, &processedSize));
    if (processedSize != kSignatureSize)
      return S_FALSE;
    if (buffer[0] != 'B' || buffer[1] != 'Z' || buffer[2] != 'h')
      return S_FALSE;

    UINT64 endPosition;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPosition));
    _item.PackSize = endPosition - _streamStartPosition;
    
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


STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UINT32(-1));
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

  extractCallback->SetTotal(_item.PackSize);

  UINT64 currentTotalPacked = 0;
  
  RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  
  CMyComPtr<ISequentialOutStream> realOutStream;
  INT32 askMode;
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
  
  RINOK(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL));

  #ifndef COMPRESS_BZIP2
  CCoderLibrary lib;
  #endif
  CMyComPtr<ICompressCoder> decoder;
  #ifdef COMPRESS_BZIP2
  decoder = new NCompress::NBZip2::CDecoder;
  #else
  RINOK(lib.LoadAndCreateCoder(
      GetBZip2CodecPath(),
      CLSID_CCompressBZip2Decoder, &decoder));
  #endif

  HRESULT result = decoder->Code(_stream, outStream, NULL, NULL, progress);
  outStream.Release();
  if (result == S_FALSE)
    RINOK(extractCallback->SetOperationResult(
        NArchive::NExtract::NOperationResult::kDataError))
  else if (result == S_OK)
    RINOK(extractCallback->SetOperationResult(
      NArchive::NExtract::NOperationResult::kOK))
  else
    return result;
 
  return S_OK;
  COM_TRY_END
}

}}
