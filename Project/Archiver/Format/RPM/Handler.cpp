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

//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UINT64 totalSize = 0;
  if(numItems == 0)
    return S_OK;
  if(numItems != 1)
    return E_FAIL;
  if (indices[0] != 0)
    return E_FAIL;

  UINT64 currentTotalSize = 0;
  
  RINOK(extractCallback->SetTotal(m_Size));
  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CComPtr<ISequentialOutStream> realOutStream;
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

  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = 
      new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, false);
  
  try
  {
    RINOK(copyCoder->Code(m_InStream, realOutStream,
        NULL, NULL, progress));
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

STDMETHODIMP CHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  UINT32 index = 0;
  return Extract(&index, 1, testMode, extractCallback);
  COM_TRY_END
}

}}
