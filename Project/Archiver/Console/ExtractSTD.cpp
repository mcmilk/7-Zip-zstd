// ExtractSTD.cpp

#include "StdAfx.h"

#include "ExtractSTD.h"
#include "ExtractCallback.h"
#include "ArError.h"

#include "Common/StdOutStream.h"
#include "../../Compress/Interface/CompressInterface.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

using namespace std;
using namespace NZipSettings;

using namespace NWindows;

static const char *kEverythingIsOk = "Everything is Ok";

HRESULT DeCompressArchiveSTD(
    IArchiveHandler200 *anArchive,
    const NWildcard::CCensor &aWildcardCensor,
    const CExtractOptions &anOptions)
{
  CRecordVector<UINT32> aRealIndexes;
  UINT32 aNumItems;
  RETURN_IF_NOT_S_OK(anArchive->GetNumberOfItems(&aNumItems));

  for(UINT32 i = 0; i < aNumItems; i++)
  {
    NCOM::CPropVariant aPropVariant;
    RETURN_IF_NOT_S_OK(anArchive->GetProperty(i, kaipidPath, &aPropVariant));
    UString aFilePath;
    if(aPropVariant.vt == VT_EMPTY)
      aFilePath = anOptions.DefaultItemName;
    else
    {
      if(aPropVariant.vt != VT_BSTR)
        return E_FAIL;
      aFilePath = aPropVariant.bstrVal;
    }
    if (!aWildcardCensor.CheckName(aFilePath))
      continue;
    aRealIndexes.Add(i);
  }
  if (aRealIndexes.Size() == 0)
    return S_OK;

  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
    new CComObjectNoLock<CExtractCallBackImp>;
  CComPtr<IExtractCallback200> anExtractCallBack(anExtractCallBackSpec);
  
  
  UStringVector aRemovePathParts;

  NZipSettings::NExtraction::CInfo anExtractionInfo;
  anExtractionInfo.PathMode = anOptions.FullPathMode() ? NExtraction::NPathMode::kFullPathnames:
      NExtraction::NPathMode::kNoPathnames;

  if (anOptions.YesToAll)
    anExtractionInfo.OverwriteMode = NExtraction::NOverwriteMode::kWithoutPrompt;
  else
  {
    anExtractionInfo.OverwriteMode = anOptions.OverwriteMode;

  }


  anExtractCallBackSpec->Init(anArchive, 
      anOptions.OutputBaseDir, anExtractionInfo, aRemovePathParts, CP_OEMCP,
      anOptions.DefaultItemName, 
      anOptions.ArchiveFileInfo.LastWriteTime,
      anOptions.ArchiveFileInfo.Attributes,
      anOptions.PasswordEnabled, 
      anOptions.Password);

  HRESULT aResult = anArchive->Extract(&aRealIndexes.Front(), 
      aRealIndexes.Size(), anOptions.ExtractMode == NExtractMode::kTest, 
      anExtractCallBack);

  if (anExtractCallBackSpec->m_NumErrors != 0)
    throw NExitCode::CMultipleErrors(anExtractCallBackSpec->m_NumErrors);

  g_StdOut << endl << kEverythingIsOk << endl;

  if (aResult != S_OK)
    throw NExitCode::CSystemError(aResult);

  return aResult;
}
