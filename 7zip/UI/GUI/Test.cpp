// Test.h

#include "StdAfx.h"

#include "Test.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/DLL.h"
#include "Windows/Thread.h"

#include "../Common/OpenArchive.h"
#include "../Common/DefaultName.h"

#ifndef EXCLUDE_COM
#include "../Common/ZipRegistry.h"
#endif

#include "../Explorer/MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "../../FileManager/ExtractCallback.h"
#include "../../FileManager/LangUtils.h"

#include "../Agent/ArchiveExtractCallback.h"

#include "resource.h"

#include "../../FileManager/OpenCallback.h"

using namespace NWindows;

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

struct CThreadTesting
{
  NDLL::CLibrary Library;
  CMyComPtr<IInArchive> Archive;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderArchiveExtractCallback> ExtractCallback2;
  CMyComPtr<IArchiveExtractCallback> ExtractCallback;

  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    Result = Archive->Extract(0, -1, BoolToInt(true), ExtractCallback);
    ExtractCallbackSpec->ProgressDialog.MyClose();
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

  COpenArchiveCallback *openCallbackSpec = new COpenArchiveCallback;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  openCallbackSpec->_passwordIsDefined = false;
  openCallbackSpec->_parentWindow = parentWindow;

  CSysString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(fileName, fullName, fileNamePartStartIndex);

  openCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  CArchiverInfo archiverInfo;
  int subExtIndex;
  RINOK(OpenArchive(fileName, 
      &tester.Library, &tester.Archive, 
      archiverInfo, subExtIndex, openCallback));

  UString defaultName = GetDefaultName(fileName, 
      archiverInfo.Extensions[subExtIndex].Extension, 
      archiverInfo.Extensions[subExtIndex].AddExtension);

  tester.ExtractCallbackSpec = new CExtractCallbackImp;
  tester.ExtractCallback2 = tester.ExtractCallbackSpec;
  
  tester.ExtractCallbackSpec->_parentWindow = 0;
  #ifdef LANG        
  const CSysString title = LangLoadString(IDS_PROGRESS_TESTING, 0x02000F90);
  #else
  const CSysString title = NWindows::MyLoadString(IDS_PROGRESS_TESTING);
  #endif
  
  tester.ExtractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, 
      openCallbackSpec->_passwordIsDefined, openCallbackSpec->_password);

  CArchiveExtractCallback *extractCallback200Spec = new CArchiveExtractCallback;
  tester.ExtractCallback = extractCallback200Spec;

  FILETIME fileTomeDefault;
  extractCallback200Spec->Init(tester.Archive, 
      tester.ExtractCallback2, 
      TEXT(""), NExtractionMode::NPath::kFullPathnames, 
      NExtractionMode::NOverwrite::kWithoutPrompt, UStringVector(),
      GetCurrentFileCodePage(), defaultName, 
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
