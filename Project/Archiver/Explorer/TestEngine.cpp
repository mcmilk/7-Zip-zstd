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
#include "../../FileManager/FormatUtils.h"

#include "../../FileManager/ExtractCallback.h"

#include "resource.h"

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

  CComObjectNoLock<CExtractCallbackImp> *extractCallbackSpec =
    new CComObjectNoLock<CExtractCallbackImp>;
  CComPtr<IExtractCallback2> extractCallback(extractCallbackSpec);
  
  extractCallbackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_TESTING);
  #endif
  extractCallbackSpec->StartProgressDialog(title);
  extractCallbackSpec->_appTitle.Window = (HWND)extractCallbackSpec->_progressDialog;
  extractCallbackSpec->_appTitle.Title = title;

  UString aPassword;
  extractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      !aPassword.IsEmpty(), aPassword);

  aResult = anArchiveHandler->Extract(
      NExtractionMode::NPath::kFullPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      L"", true, extractCallback);

  if (extractCallbackSpec->_messages.IsEmpty())
  {
    extractCallbackSpec->DestroyWindows();
    MessageBox(0, LangLoadString(IDS_MESSAGE_NO_ERRORS, 0x02000608),
      LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90), 0);
  }
  return aResult;
}
