// ExtractEngine.h

#include "StdAfx.h"

#include "ExtractEngine.h"

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"

#include "../../Common/OpenEngine200.h"

#include "../../Explorer/MyMessages.h"
#include "../../../FileManager/FormatUtils.h"

#include "ExtractCallback.h"

using namespace NWindows;

HRESULT ExtractArchive(const CSysString &aFileName, const CSysString &aFolderName)
{
  CComPtr<IArchiveHandler200> anArchiveHandler;
  NZipRootRegistry::CArchiverInfo anArchiverInfoResult;

  HRESULT aResult = OpenArchive(aFileName, &anArchiveHandler, 
      anArchiverInfoResult);
  if (aResult != S_OK)
    return aResult;

  #ifndef  NO_REGISTRY
  CZipRegistryManager aZipRegistryManager;
  #endif

  CSysString directoryPath = aFolderName;
  NFile::NName::NormalizeDirPathPrefix(directoryPath);

  /*
  CSysString directoryPath;
  {
    CSysString aFullPath;
    int aFileNamePartStartIndex;
    if (!NWindows::NFile::NDirectory::MyGetFullPathName(aFileName, aFullPath, aFileNamePartStartIndex))
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
  
  CComObjectNoLock<CExtractCallBackImp> *anExtractCallBackSpec =
    new CComObjectNoLock<CExtractCallBackImp>;

  CComPtr<IExtractCallback200> anExtractCallBack(anExtractCallBackSpec);
  
  anExtractCallBackSpec->StartProgressDialog();

  // anExtractCallBackSpec->m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);

  NFile::NFind::CFileInfo anArchiveFileInfo;
  if (!NFile::NFind::FindFile(aFileName, anArchiveFileInfo))
    throw "there is no archive file";

  anExtractCallBackSpec->Init(anArchiveHandler, directoryPath,
      L"Default", anArchiveFileInfo.LastWriteTime, 0);

  return anArchiveHandler->ExtractAllItems(BoolToMyBool(false), anExtractCallBack);
}



