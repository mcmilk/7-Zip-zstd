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

#include "../../Compress/Copy/CopyCoder.h"
#include "../Common/ItemNameUtils.h"

using namespace NWindows;

namespace NArchive {
namespace NRpm {

STATPROPSTG kProps[] = 
{
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  try
  {
    if(OpenArchive(inStream) != S_OK)
      return S_FALSE;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_Pos));
    m_InStream = inStream;
    UInt64 endPosition;
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
    case kpidSize:
    case kpidPackedSize:
      prop = m_Size;
      break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = 1;
  if(numItems == 0)
    return S_OK;
  if(numItems != 1)
    return E_FAIL;
  if (indices[0] != 0)
    return E_FAIL;

  bool testMode = (_aTestMode != 0);
  
  UInt64 currentTotalSize = 0;
  RINOK(extractCallback->SetTotal(m_Size));
  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode = testMode ? 
      NArchive::NExtract::NAskMode::kTest :
      NArchive::NExtract::NAskMode::kExtract;
  Int32 index = 0;
 
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

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);
  
  RINOK(copyCoder->Code(m_InStream, realOutStream, NULL, NULL, progress));
  realOutStream.Release();
  return extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK);
  COM_TRY_END
}

}}
