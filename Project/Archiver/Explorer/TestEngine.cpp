// TestEngine.h

#include "StdAfx.h"

#include "TestEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"

#include "../Common/OpenEngine2.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

#include "MyMessages.h"
#include "FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

HRESULT TestArchive(HWND aParentWindow, const CSysString &aFileName)
{
  CComPtr<IArchiveHandler100> anArchiveHandler;
  NZipRootRegistry::CArchiverInfo anArchiverInfoResult;

  UString aDefaultName;
  HRESULT aResult = OpenArchive(aFileName, &anArchiveHandler, 
      anArchiverInfoResult, aDefaultName, NULL);
  if (aResult != S_OK)
    return aResult;

  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
    new CComObjectNoLock<CExtractCallBackImp>;
  CComPtr<IExtractCallback2> anExtractCallBack(anExtractCallBackSpec);
  
  anExtractCallBackSpec->m_ParentWindow = 0;
  #ifdef LANG        
  const CSysString aTitle = LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90);
  #else
  const CSysString aTitle = NWindows::MyLoadString(IDS_PROGRESS_TESTING);
  #endif
  anExtractCallBackSpec->StartProgressDialog(aTitle);
  UString aPassword;
  anExtractCallBackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      !aPassword.IsEmpty(), aPassword);

  aResult = anArchiveHandler->Extract(
      NExtractionMode::NPath::kFullPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      L"", true, anExtractCallBack);

  if (anExtractCallBackSpec->m_Messages.IsEmpty())
  {
    anExtractCallBackSpec->DestroyWindows();
    MessageBox(0, LangLoadString(IDS_MESSAGE_NO_ERRORS, 0x02000608),
      LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90), 0);
  }
  return aResult;
}
