// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"

#include "../../Common/OpenEngine200.h"

#include "../../Explorer/MyMessages.h"
#include "../../../FileManager/FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

struct CThreadExtracting
{
  CComPtr<IArchiveHandler200> ArchiveHandler;
  CComObjectNoLock<CExtractCallbackImp> *ExtractCallbackSpec;
  CComPtr<IExtractCallback200> ExtractCallback;

  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->_progressDialog._dialogCreatedEvent.Lock();
    Result = ArchiveHandler->ExtractAllItems(BoolToInt(false), 
        ExtractCallback);
    ExtractCallbackSpec->_progressDialog.MyClose();
    return 0;
  }
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtracting *)param)->Process();
  }
};


HRESULT ExtractArchive(const CSysString &fileName, const CSysString &folderName)
{
  CThreadExtracting extracter;

  NZipRootRegistry::CArchiverInfo archiverInfoResult;
  HRESULT result = OpenArchive(fileName, &extracter.ArchiveHandler, 
      archiverInfoResult);
  if (result != S_OK)
    return result;

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
    MyMessageBox(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 
        #ifdef LANG        
        0x02000603, 
        #endif 
        GetUnicodeString((LPCTSTR)directoryPath)));
    return E_FAIL;
  }
  
  extracter.ExtractCallbackSpec = new CComObjectNoLock<CExtractCallbackImp>;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  
  // anExtractCallBackSpec->StartProgressDialog();

  // anExtractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  NFile::NFind::CFileInfo archiveFileInfo;
  if (!NFile::NFind::FindFile(fileName, archiveFileInfo))
    throw "there is no archive file";

  extracter.ExtractCallbackSpec->Init(extracter.ArchiveHandler, 
      directoryPath, L"Default", archiveFileInfo.LastWriteTime, 0);

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
}



