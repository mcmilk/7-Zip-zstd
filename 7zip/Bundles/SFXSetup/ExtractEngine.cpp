// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"

#include "../../UI/Common/OpenArchive.h"

#include "../../UI/Explorer/MyMessages.h"
#include "../../FileManager/FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

struct CThreadExtracting
{
  CMyComPtr<IInArchive> ArchiveHandler;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IArchiveExtractCallback> ExtractCallback;

  #ifndef _NO_PROGRESS
  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    Result = ArchiveHandler->Extract(0, (UINT32)-1 , BoolToInt(false), 
        ExtractCallback);
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtracting *)param)->Process();
  }
  #endif
};

static const LPCTSTR kCantFindArchive = TEXT("Can not find archive file");
static const LPCTSTR kCantOpenArchive = TEXT("File is not correct archive");

HRESULT ExtractArchive(
    const CSysString &fileName, 
    const CSysString &folderName
    #ifdef _SILENT
    , CSysString &resultMessage
    #endif
    )
{
  NFile::NFind::CFileInfo archiveFileInfo;
  if (!NFile::NFind::FindFile(fileName, archiveFileInfo))
  {
    #ifndef _SILENT
    MessageBox(0, kCantFindArchive, TEXT("7-Zip"), 0);
    #else
    resultMessage = kCantFindArchive;
    #endif
    return E_FAIL;
  }

  CThreadExtracting extracter;

  CArchiverInfo archiverInfoResult;
  HRESULT result = OpenArchive(fileName, &extracter.ArchiveHandler, 
      archiverInfoResult, NULL);
  if (result != S_OK)
  {
    #ifdef _SILENT
    resultMessage = kCantOpenArchive;
    #endif
    return E_FAIL;
  }

  CSysString directoryPath = folderName;
  NFile::NName::NormalizeDirPathPrefix(directoryPath);

  /*
  CSysString directoryPath;
  {
    CSysString aFullPath;
    int aFileNamePartStartIndex;
    if (!NWindows::NFile::NDirectory::MyGetFullPathName(fileName, aFullPath, aFileNamePartStartIndex))
    {
      MessageBox(NULL, "Error 1329484", "7-Zip", 0);
      return E_FAIL;
    }
    directoryPath = aFullPath.Left(aFileNamePartStartIndex);
  }
  */

  if(!NFile::NDirectory::CreateComplexDirectory(directoryPath))
  {
    #ifndef _SILENT
    MyMessageBox(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 
        #ifdef LANG        
        0x02000603, 
        #endif 
        GetUnicodeString((LPCTSTR)directoryPath)));
    #else
    resultMessage = TEXT("Can not create output folder");
    #endif
    return E_FAIL;
  }
  
  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  
  // anExtractCallBackSpec->StartProgressDialog();

  // anExtractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  extracter.ExtractCallbackSpec->Init(extracter.ArchiveHandler, 
      directoryPath, L"Default", archiveFileInfo.LastWriteTime, 0);

  #ifndef _NO_PROGRESS

  CThread thread;
  if (!thread.Create(CThreadExtracting::MyThreadFunction, &extracter))
    throw 271824;

  CSysString title;
  #ifdef LANG        
  title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  title = NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING);
  #endif
  extracter.ExtractCallbackSpec->StartProgressDialog(title);
  return extracter.Result;

  #else

  result = extracter.ArchiveHandler->Extract(0, (UINT32)-1,
        BoolToInt(false), extracter.ExtractCallback);
  #ifdef _SILENT
  resultMessage = extracter.ExtractCallbackSpec->_message;
  #endif
  return result;
  #endif
}



