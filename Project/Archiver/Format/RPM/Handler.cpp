// RPM/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Interface/StreamObjects.h"
#include "Interface/EnumStatProp.h"
#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"

#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"
#include "Windows/Defs.h"

#include "Compression/CopyCoder.h"
#include "Archive/Common/ItemNameUtils.h"
#include "Archive/RPM/InEngine.h"



using namespace NWindows;

namespace NArchive {
namespace NRPM {

STATPROPSTG kProperties[] = 
{
//  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8}
};


STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    if(OpenArchive(aStream) != S_OK)
      return S_FALSE;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_Pos));
    m_InStream = aStream;
    UINT64 anEndPosition;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_END, &anEndPosition));
    m_Size = anEndPosition - m_Pos;
    return S_OK;
  }
  catch(...)
  {
    return S_FALSE;
  }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  m_InStream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(
    UINT32 anIndex, 
    PROPID aPropID,  
    PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;

  switch(aPropID)
  {
    /*
    case kpidPath:
      aPropVariant = (const wchar_t *)L"a.cpio.gz";
      break;
    */
    case kpidIsFolder:
      aPropVariant = false;
      break;
    case kpidSize:
    case kpidPackedSize:
      aPropVariant = m_Size;
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aTotalSize = 0;
  if(aNumItems == 0)
    return S_OK;
  if(aNumItems != 1)
    return E_FAIL;
  if (anIndexes[0] != 0)
    return E_FAIL;

  UINT64 aCurrentTotalSize = 0;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->SetTotal(m_Size));
  RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalSize));
  CComPtr<ISequentialOutStream> aRealOutStream;
  INT32 anAskMode;
  anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
      NArchiveHandler::NExtract::NAskMode::kExtract;
  INT32 anIndex = 0;
 
  RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

  if(!aTestMode && (!aRealOutStream))
    return S_OK;

  RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));

  if (aTestMode)
  {
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
    return S_OK;
  }

  RETURN_IF_NOT_S_OK(m_InStream->Seek(m_Pos, STREAM_SEEK_SET, NULL));

  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> aCopyCoder = aCopyCoderSpec;

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anExtractCallBack, false);
  
  try
  {
    RETURN_IF_NOT_S_OK(aCopyCoder->Code(m_InStream, aRealOutStream,
        NULL, NULL, aProgress));
  }
  catch(...)
  {
    aRealOutStream.Release();
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
    return S_OK;
  }
  aRealOutStream.Release();
  RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  UINT32 anIndex = 0;
  return Extract(&anIndex, 1, aTestMode, anExtractCallBack);
  COM_TRY_END
}

}}
