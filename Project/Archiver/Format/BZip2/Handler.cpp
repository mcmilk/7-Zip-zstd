// Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/EnumStatProp.h"
#include "Interface/StreamObjects.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "Windows/COMTry.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../Common/DummyOutStream.h"

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Decoder.h"
#else
// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

using namespace NWindows;

namespace NArchive {
namespace NBZip2 {

static const kNumItemInArchive = 1;


STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  // { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
};

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = kNumItemInArchive;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(
    UINT32 anIndex, 
    PROPID aPropID,  
    PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  if (anIndex != 0)
    return E_INVALIDARG;
  switch(aPropID)
  {
    case kpidIsFolder:
      aPropVariant = false;
      break;
    case kpidPackedSize:
      aPropVariant = m_Item.PackSize;
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
    const kSignatureSize = 3;
    BYTE aBuffer[kSignatureSize];
    UINT32 aProcessedSize;
    RETURN_IF_NOT_S_OK(aStream->Read(aBuffer, kSignatureSize, &aProcessedSize));
    if (aProcessedSize != kSignatureSize)
      return S_FALSE;
    if (aBuffer[0] != 'B' || aBuffer[1] != 'Z' || aBuffer[2] != 'h')
      return S_FALSE;

    UINT64 anEndPosition;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_END, &anEndPosition));
    m_Item.PackSize = anEndPosition - m_StreamStartPosition;
    
    m_Stream = aStream;
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
  m_Stream.Release();
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  if (aNumItems == 0)
    return S_OK;
  if (aNumItems != kNumItemInArchive)
    return E_INVALIDARG;
  if (anIndexes[0] != 0)
    return E_INVALIDARG;

  bool aTestMode = (_aTestMode != 0);

  anExtractCallBack->SetTotal(m_Item.PackSize);

  UINT64 aCurrentTotalPacked = 0;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalPacked));
  
  CComPtr<ISequentialOutStream> aRealOutStream;
  INT32 anAskMode;
  anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
  NArchiveHandler::NExtract::NAskMode::kExtract;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(0, &aRealOutStream, anAskMode));
  
  
  if(!aTestMode && !aRealOutStream)
    return S_OK;

  anExtractCallBack->PrepareOperation(anAskMode);

  CComObjectNoLock<CDummyOutStream> *anOutStreamSpec = 
    new CComObjectNoLock<CDummyOutStream>;
  CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
  anOutStreamSpec->Init(aRealOutStream);
  
  aRealOutStream.Release();

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anExtractCallBack, true);
  
  RETURN_IF_NOT_S_OK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));

  CComPtr<ICompressCoder> aDecoder;
  #ifdef COMPRESS_BZIP2
  aDecoder = new CComObjectNoLock<NCompress::NBZip2::NDecoder::CCoder>;
  #else
  RETURN_IF_NOT_S_OK(aDecoder.CoCreateInstance(CLSID_CCompressBZip2Decoder));
  #endif

  HRESULT aResult = aDecoder->Code(m_Stream, anOutStream, NULL, NULL, aProgress);
  anOutStream.Release();
  if (aResult == S_FALSE)
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
        NArchiveHandler::NExtract::NOperationResult::kDataError))
  else if (aResult == S_OK)
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
      NArchiveHandler::NExtract::NOperationResult::kOK))
  else
    return aResult;
 
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode, IExtractCallback200 *anExtractCallBack)
{
  UINT32 anIndex = 0;
  return Extract(&anIndex, 1, aTestMode, anExtractCallBack);
}

}}
