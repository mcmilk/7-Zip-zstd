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
  const CSysString aTitle = LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90);
  #else
  const CSysString aTitle = NWindows::MyLoadString(IDS_PROGRESS_TESTING);
  #endif
  extractCallbackSpec->StartProgressDialog(aTitle);
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
