// RPM/Handler.cpp

#include "StdAfx.h"

#include "RpmHandler.h"
#include "RpmIn.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"
#include "Common/ComTry.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"

#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"
// #include "Interface/EnumStatProp.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../Common/ItemNameUtils.h"

using namespace NWindows;

namespace NArchive {
namespace NRpm {

STATPROPSTG kProperties[] = 
{
//  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8}
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

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    if(OpenArchive(inStream) != S_OK)
      return S_FALSE;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Pos));
    m_InStream = inStream;
    UINT64 endPosition;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &endPosition));
    m_Size = endPosition - m_Pos;
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

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;

  switch(propID)
  {
    /*
    case kpidPath:
      propVariant = (const wchar_t *)L"a.cpio.gz";
      break;
    */
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidSize:
    case kpidPackedSize:
      propVariant = m_Size;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UINT32(-1));
  if (allFilesMode)
    numItems = 1;
  if(numItems == 0)
    return S_OK;
  if(numItems != 1)
    return E_FAIL;
  if (indices[0] != 0)
    return E_FAIL;

  bool testMode = (_aTestMode != 0);
  
  UINT64 currentTotalSize = 0;
  RINOK(extractCallback->SetTotal(m_Size));
  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CMyComPtr<ISequentialOutStream> realOutStream;
  INT32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
      NArchive::NExtract::NAskMode::kExtract;
  INT32 index = 0;
 
  RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

  if(!testMode && (!realOutStream))
    return S_OK;

  RINOK(extractCallback->PrepareOperation(askMode));

  if (testMode)
  {
    RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    return S_OK;
  }

  RINOK(m_InStream->Seek(m_Pos, STREAM_SEEK_SET, NULL));

  CMyComPtr<ICompressCoder> copyCoder = new NCompress::CCopyCoder;

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, false);
  
  try
  {
    RINOK(copyCoder->Code(m_InStream, realOutStream, NULL, NULL, progress));
  }
  catch(...)
  {
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
    return S_OK;
  }
  realOutStream.Release();
  RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
  return S_OK;
  COM_TRY_END
}

}}
