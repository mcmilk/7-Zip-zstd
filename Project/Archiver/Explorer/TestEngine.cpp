// TestEngine.h

#include "StdAfx.h"

#include "TestEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/COM.h"
#include "Windows/Thread.h"

#include "../Common/OpenEngine200.h"

#ifndef  NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

#include "MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "../../FileManager/ExtractCallback.h"
#include "../../FileManager/LangUtils.h"

#include "../Agent/ExtractCallback200.h"

#include "resource.h"

using namespace NWindows;

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

struct CThreadTesting
{
  CComPtr<IArchiveHandler200> ArchiveHandler;
  CComObjectNoLock<CExtractCallbackImp> *ExtractCallbackSpec;
  CComPtr<IExtractCallback2> ExtractCallback;
  CComPtr<IExtractCallback200> ExtractCallback200;

  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    ExtractCallbackSpec->_progressDialog._dialogCreatedEvent.Lock();
    ExtractCallbackSpec->_appTitle.Window = 
        (HWND)ExtractCallbackSpec->_progressDialog;
    Result = ArchiveHandler->ExtractAllItems(BoolToInt(true), 
        ExtractCallback200);
    ExtractCallbackSpec->_progressDialog.MyClose();
    return 0;
  }
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadTesting *)param)->Process();
  }
};

HRESULT TestArchive(HWND parentWindow, const CSysString &fileName)
{
  CThreadTesting tester;

  NZipRootRegistry::CArchiverInfo archiverInfoResult;

  RETURN_IF_NOT_S_OK(OpenArchive(fileName, &tester.ArchiveHandler, 
      archiverInfoResult, NULL));

  tester.ExtractCallbackSpec = new CComObjectNoLock<CExtractCallbackImp>;
  tester.ExtractCallback = tester.ExtractCallbackSpec;
  
  tester.ExtractCallbackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_TESTING);
  #endif
  
  // tester.ExtractCallbackSpec->_appTitle.Window = (HWND)extractCallbackSpec->_progressDialog;
  tester.ExtractCallbackSpec->_appTitle.Window = 0;
  
  tester.ExtractCallbackSpec->_appTitle.Title = title;

  UString password;
  tester.ExtractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      !password.IsEmpty(), password);

  CComObjectNoLock<CExtractCallBack200Imp> *extractCallback200Spec = new 
      CComObjectNoLock<CExtractCallBack200Imp>;
  tester.ExtractCallback200 = extractCallback200Spec;

  FILETIME fileTomeDefault;
  extractCallback200Spec->Init(tester.ArchiveHandler, tester.ExtractCallback, 
      TEXT(""), NExtractionMode::NPath::kFullPathnames, 
      NExtractionMode::NOverwrite::kWithoutPrompt, UStringVector(),
      GetCurrentFileCodePage(), L"", 
      fileTomeDefault, 0);


  CThread thread;
  if (!thread.Create(CThreadTesting::MyThreadFunction, &tester))
    throw 271824;
  tester.ExtractCallbackSpec->StartProgressDialog(title);

  if (tester.Result == S_OK && tester.ExtractCallbackSpec->_messages.IsEmpty())
  {
    // extractCallbackSpec->DestroyWindows();
    MessageBox(0, LangLoadString(IDS_MESSAGE_NO_ERRORS, 0x02000608),
      LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90), 0);
  }
  return tester.Result;
}
