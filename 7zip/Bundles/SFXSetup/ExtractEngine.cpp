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
  // CMyComPtr<IInArchive> ArchiveHandler;
  CArchiveLink ArchiveLink;

  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IArchiveExtractCallback> ExtractCallback;

  #ifndef _NO_PROGRESS
  HRESULT Result;
  
  DWORD Process()
  {
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    Result = ArchiveLink.GetArchive()->Extract(0, (UInt32)-1 , BoolToInt(false), 
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
    const UString &fileName, 
    const UString &folderName,
    COpenCallbackGUI *openCallback
    #ifdef _SILENT
    , UString &resultMessage
    #endif
    )
{
  NFile::NFind::CFileInfoW archiveFileInfo;
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

  HRESULT result = MyOpenArchive(fileName, extracter.ArchiveLink, openCallback);

  /*
  CArchiverInfo archiverInfoResult;
  int subExtIndex;
  HRESULT result = OpenArchive(fileName, &extracter.ArchiveHandler, 
      archiverInfoResult, subExtIndex, NULL);
  */
  if (result != S_OK)
  {
    #ifdef _SILENT
    resultMessage = kCantOpenArchive;
    #endif
    return E_FAIL;
  }

  UString directoryPath = folderName;
  NFile::NName::NormalizeDirPathPrefix(directoryPath);

  /*
  UString directoryPath;
  {
    UString fullPath;
    int fileNamePartStartIndex;
    if (!NWindows::NFile::NDirectory::MyGetFullPathName(fileName, fullPath, fileNamePartStartIndex))
    {
      MessageBox(NULL, "Error 1329484", "7-Zip", 0);
      return E_FAIL;
    }
    directoryPath = fullPath.Left(fileNamePartStartIndex);
  }
  */

  if(!NFile::NDirectory::CreateComplexDirectory(directoryPath))
  {
    #ifndef _SILENT
    MyMessageBox(MyFormatNew(IDS_CANNOT_CREATE_FOLDER, 
        #ifdef LANG        
        0x02000603, 
        #endif 
        directoryPath));
    #else
    resultMessage = TEXT("Can not create output folder");
    #endif
    return E_FAIL;
  }
  
  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  
  // anExtractCallBackSpec->StartProgressDialog();

  // anExtractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  extracter.ExtractCallbackSpec->Init(
      extracter.ArchiveLink.GetArchive(), 
      directoryPath, L"Default", archiveFileInfo.LastWriteTime, 0);

  #ifndef _NO_PROGRESS

  CThread thread;
  if (!thread.Create(CThreadExtracting::MyThreadFunction, &extracter))
    throw 271824;

  UString title;
  #ifdef LANG        
  title = LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890);
  #else
  title = NWindows::MyLoadStringW(IDS_PROGRESS_EXTRACTING);
  #endif
  extracter.ExtractCallbackSpec->StartProgressDialog(title);
  return extracter.Result;

  #else

  result = extracter.ArchiveHandler->Extract(0, (UInt32)-1,
        BoolToInt(false), extracter.ExtractCallback);
  #ifdef _SILENT
  resultMessage = extracter.ExtractCallbackSpec->_message;
  #endif
  return result;
  #endif
}



