// ZipViewDelete.cpp

#include "StdAfx.h"

#include "ZipViewObject.h"
#include "ZipViewUtils.h"
#include "DeleteEngine.h"

#include "../Common/UpdateUtils.h"

#include "Common/StringConvert.h"

#include "Interface/FileStreams.h"

#include "Windows/FileDir.h"

using namespace NWindows;

/*

static LPCTSTR kTempArcivePrefix = _T("7zi");

void CZipViewObject::CommandDelete()
{
  {
  HWND aBrowserWindow;
  if(m_ShellBrowser->GetWindow(&aBrowserWindow) != S_OK)
    return;
  
  vector<CDirOrFileIndex> aLocalIndexes;
  if(::GetFocus() == HWND(m_ListView))
    GetSelectedItemsIndexes(aLocalIndexes);
  if(aLocalIndexes.size() == 0)
  {
    ::MessageBeep(MB_OK);
    return;
  }
  CString aText;
  UINT aCaptionID;
  if(aLocalIndexes.size() == 1)
  {
    CDirOrFileIndex anIndex = aLocalIndexes[0];
    UINT aTextID;
    const UString* aName;
    if(anIndex.IsDirIndex())
    {
      aName = &m_ArchiveFolderItem->m_DirSubItems[anIndex.GetAsDirIndex()].m_Name;
      aCaptionID = IDS_CONFIRM_FOLDER_DELETE_CAPTION;
      aTextID = IDS_CONFIRM_FOLDER_DELETE;
    }
    else
    {
      aName = &m_ArchiveFolderItem->m_FileSubItems[anIndex.GetAsFileIndex()].m_Name;
      aCaptionID = IDS_CONFIRM_FILE_DELETE_CAPTION;
      aTextID = IDS_CONFIRM_FILE_DELETE;
    }
    AfxFormatString1(aText, aTextID, GetSystemString(*aName));
  }
  else
  {
    aCaptionID  = IDS_CONFIRM_ITEMS_DELETE_CAPTION;
    aText.Format(IDS_CONFIRM_ITEMS_DELETE, aLocalIndexes.size());
  }
  CString aCaption;
  aCaption.LoadString(aCaptionID);
  if(::MessageBox(m_hWnd, aText, aCaption, MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  CComPtr<IOutArchiveHandler> anOutArchive;

  HRESULT aResult = m_ProxyHandler->m_ArchiveHandler.QueryInterface(&anOutArchive);
  if(aResult != S_OK)
  {
    AfxMessageBox(IDS_QUERY_UPDATE_INTERFACE_FAIL);
    return;
  }

  CComObjectNoLock<CDeleteCallBackImp> *anUpdateCallBackSpec =
    new CComObjectNoLock<CDeleteCallBackImp>;
  CComPtr<IUpdateCallBack> anUpdateCallBack(anUpdateCallBackSpec );

  if(anUpdateCallBackSpec->m_ProcessDialog.Create(IDD_DIALOG_PROGRESS) == FALSE)
    throw 112102;
  CShellBrowserDisabler aWndEnabledRestorer(m_ShellBrowser);
  anUpdateCallBackSpec->m_ProcessDialog.ShowWindow(SW_SHOWNORMAL);
  
  anUpdateCallBackSpec->Init();
  vector<int> aRealIndexes;
  AddAllRealIndexes(aLocalIndexes, aRealIndexes);

  

  CZipRegistryManager aZipRegistryManager;
  NZipSettings::NWorkDir::CInfo aWorkDirInfo;
  aZipRegistryManager.ReadWorkDirInfo(aWorkDirInfo);

  CSysString aWorkDir = GetWorkDir(aWorkDirInfo, m_ProxyHandler->m_FileName);
  NFile::NDirectory::CreateComplexDirectory(aWorkDir);

  NFile::NDirectory::CTempFile aTempFile;
  CSysString aTempFileName;
  if (aTempFile.Create(aWorkDir, kTempArcivePrefix, aTempFileName) == 0)
    return;
  {
    CComObjectNoLock<COutFileStream> *anOutStreamSpec =
      new CComObjectNoLock<COutFileStream>;
    CComPtr<IOutStream> anOutStream(anOutStreamSpec);
    anOutStreamSpec->Open(aTempFileName);
    
    aResult = anOutArchive->DeleteItems(anOutStream, &aRealIndexes.front(), 
       aRealIndexes.size(), anUpdateCallBack);
    if (aResult != S_OK)
    {
      ShowErrorMessage(aResult);
      return;
    }
  }

  ////////////////////////////
  // Save ZipView FolderItem;
  UStringVector aPathVector;
  CArchiveFolderItem *anItem = m_ArchiveFolderItem;
  while(anItem->m_Parent != NULL)
  {
    aPathVector.Add(anItem->m_Name);
    anItem = anItem->m_Parent;
  }
  /////////////////////////////////



  anOutArchive.Release();
  m_ProxyHandler->m_ArchiveHandler->CloseArchive();
  
  if (!NFile::NDirectory::DeleteFileAlways(m_ProxyHandler->m_FileName))
  {
    ShowLastErrorMessage();
    return;
  }
  aTempFile.DisableDeleting();
  if (!::MoveFile(aTempFileName, m_ProxyHandler->m_FileName))
  {
    ShowLastErrorMessage();
    return;
  }

  aResult = ReOpenArchive(m_ProxyHandler->m_ArchiveHandler, m_ProxyHandler->m_FileName);
  if (aResult != S_OK)
  {
    ShowErrorMessage(aResult);
    return;
  }

  if(!m_ProxyHandler->ReInit())
  {
    return;
  }

  ////////////////////////////
  // Restore ZipView FolderItem;
  anItem = &m_ProxyHandler->m_FolderItemHead;
  while (aPathVector.Size() > 0)
  {
    anItem = anItem->FindDirSubItem(aPathVector.Back());
    if(anItem == NULL)
      return; // It's BUG
    aPathVector.DeleteBack();
  }
  SetFolderItem(anItem);
  /////////////////////////////////


  InitListCtrl();
  RefreshListCtrl();

  ITEMIDLIST aList;
  aList.mkid.cb = 0;
  ::SHChangeNotify(SHCNE_UPDATEDIR , SHCNF_IDLIST | SHCNF_FLUSH , &aList, 0);

  }
  // for(int i = 0; i < 170000000; i++);


  // LPCITEMIDLIST anIDListCur1 = m_ArchiveFolderItem->m_DirSubItems[0].m_Properties;
  // ::SHChangeNotify(SHCNE_UPDATEDIR , SHCNF_IDLIST | SHCNF_FLUSH , anIDListCur1, 0);

  // ::SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSH, "C:\\TMP", 0);
  
  //::SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_IDLIST | SHCNF_FLUSH, 
  // (LPCITEMIDLIST)m_ProxyHandler->m_AbsoluteIDList, 0);
  
  return;
}
*/