// BZip2Handler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"
#include "../../Common/CreateCoder.h"
#include "../Common/DummyOutStream.h"

#include "BZip2Handler.h"

using namespace NWindows;

namespace NArchive {
namespace NBZip2 {

static const CMethodId kMethodId_BZip2 = 0x040202;

STATPROPSTG kProps[] = 
{
  { NULL, kpidPackedSize, VT_UI8}
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
    case kpidPackedSize: prop = _item.PackSize; break;
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
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition));
    const int kSignatureSize = 3;
    Byte buffer[kSignatureSize];
    UInt32 processedSize;
    RINOK(ReadStream(stream, buffer, kSignatureSize, &processedSize));
    if (processedSize != kSignatureSize)
      return S_FALSE;
    if (buffer[0] != 'B' || buffer[1] != 'Z' || buffer[2] != 'h')
      return S_FALSE;

    UInt64 endPosition;
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


STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UInt32(-1));
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

  UInt64 currentTotalPacked = 0;
  
  RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode;
  askMode = testMode ? NExtract::NAskMode::kTest :
  NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
    
  if(!testMode && !realOutStream)
    return S_OK;


  extractCallback->PrepareOperation(askMode);

  CMyComPtr<ICompressCoder> decoder;
  HRESULT loadResult = CreateCoder(
      EXTERNAL_CODECS_VARS
      kMethodId_BZip2, decoder, false);
  if (loadResult != S_OK || !decoder)
  {
    RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
    return S_OK;
  }

  #ifdef COMPRESS_MT
  {
    CMyComPtr<ICompressSetCoderMt> setCoderMt;
    decoder.QueryInterface(IID_ICompressSetCoderMt, &setCoderMt);
    if (setCoderMt)
    {
      RINOK(setCoderMt->SetNumberOfThreads(_numThreads));
    }
  }
  #endif

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);
  
  RINOK(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL));

  HRESULT result = S_OK;

  bool firstItem = true;
  for (;;)
  {
    lps->InSize = currentTotalPacked;
    lps->OutSize = outStreamSpec->GetSize();

    RINOK(lps->SetCur());

    const int kSignatureSize = 3;
    Byte buffer[kSignatureSize];
    UInt32 processedSize;
    RINOK(ReadStream(_stream, buffer, kSignatureSize, &processedSize));
    if (processedSize < kSignatureSize)
    {
      if (firstItem)
        return E_FAIL;
      break;
    }
    if (buffer[0] != 'B' || buffer[1] != 'Z' || buffer[2] != 'h')
    {
      if (firstItem)
        return E_FAIL;
      break;
    }
    firstItem = false;

    UInt64 dataStartPos;
    RINOK(_stream->Seek((UInt64)(Int64)(-3), STREAM_SEEK_CUR, &dataStartPos));

    result = decoder->Code(_stream, outStream, NULL, NULL, progress);

    if (result != S_OK)
      break;

    CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
    decoder.QueryInterface(IID_ICompressGetInStreamProcessedSize, &getInStreamProcessedSize);
    if (!getInStreamProcessedSize)
      break;
    UInt64 packSize;
    RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&packSize));
    UInt64 pos;
    RINOK(_stream->Seek(dataStartPos + packSize, STREAM_SEEK_SET, &pos));
    currentTotalPacked = pos - _streamStartPosition;
  }
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

IMPL_ISetCompressCodecsInfo

}}
