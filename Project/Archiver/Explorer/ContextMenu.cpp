// ContextMenu.cpp

#include "StdAfx.h"

#include "ContextMenu.h"

#include "Windows/Shell.h"
#include "Windows/Memory.h"
#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/System.h"

#include "Windows/Menu.h"
#include "Windows/ResourceString.h"

#include "resource.h"
#include "ExtractEngine.h"
#include "CompressEngine.h"
#include "Common/StringConvert.h"

#include "MyMessages.h"

using namespace NWindows;

static LPCTSTR kFileClassIDString = _T("SevenZip");

///////////////////////////////
// IShellExtInit

HRESULT CZipContextMenu::GetFileNames(LPDATAOBJECT aDataObject, 
    CSysStringVector &aFileNames)
{
  aFileNames.Clear();
  if(aDataObject == NULL)
    return E_FAIL;
  FORMATETC fmte = {CF_HDROP,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  NCOM::CStgMedium aStgMedium;
  HRESULT aResult = aDataObject->GetData(&fmte, &aStgMedium);
  if (aResult != S_OK)
    return aResult;
  aStgMedium.m_MustBeReleased = true;

  NShell::CDrop aDrop(false);
  NMemory::CGlobalLock aGlobalLock(aStgMedium->hGlobal);
  aDrop.Attach((HDROP)aGlobalLock.GetPointer());
  aDrop.QueryFileNames(aFileNames);

  return S_OK;
}

STDMETHODIMP CZipContextMenu::Initialize(LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT aDataObject, HKEY hkeyProgID)
{
  /*
  m_IsFolder = false;
  if (pidlFolder == 0)
  */
  // pidlFolder is NULL :(
  return GetFileNames(aDataObject, m_FileNames);
}


/////////////////////////////
// IContextMenu

static LPCTSTR kMainVerb = _T("SevenOpen");
static LPCTSTR kExtractVerb = _T("SevenExtract");
static LPCTSTR kCompressVerb = _T("SevenCompress");
static LPCTSTR kOpenVerb = _T("SevenOpen");

STDMETHODIMP CZipContextMenu::QueryContextMenu(HMENU hMenu, UINT anIndexMenu,
      UINT aCommandIDFirst, UINT aCommandIDLast, UINT aFlags)
{
  if(m_FileNames.Size() == 0)
    return E_FAIL;
  UINT aCurrentCommandID = aCommandIDFirst; 
  if ((aFlags & 0x000F) != CMF_NORMAL  &&
      (aFlags & CMF_VERBSONLY) == 0 &&
      (aFlags & CMF_EXPLORE) == 0) 
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, aCurrentCommandID); 

  m_CommandMap.clear();
  CCommandMapItem aCommandMapItem;

  CMenu aPopupMenu;
  if(!aPopupMenu.CreatePopup())
    throw 210503;

  aCommandMapItem.CommandInternalID = kCommandInternalNULL;
  aCommandMapItem.Verb = kMainVerb;
  aCommandMapItem.HelpString = MyLoadString(IDS_CONTEXT_CAPTION_HELP);
  m_CommandMap.push_back(aCommandMapItem);

  MENUITEMINFO aMenuInfo;
  aMenuInfo.cbSize = sizeof(aMenuInfo);
  aMenuInfo.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
  aMenuInfo.fType = MFT_STRING;
  aMenuInfo.wID = aCurrentCommandID++; 

  int anSubMenuIndex = 0;
  if(m_FileNames.Size() == 1 && aCurrentCommandID <= aCommandIDLast)
  {
    const CSysString &aFileName = m_FileNames.Front();
  
    NFile::NFind::CFileInfo aFileInfo;
    if (!NFile::NFind::FindFile(m_FileNames.Front(), aFileInfo))
      return E_FAIL;
    if (!aFileInfo.IsDirectory())
    {
      //////////////////////////
      // Open command
      aCommandMapItem.CommandInternalID = kCommandInternalIDOpen;
      aCommandMapItem.Verb = kOpenVerb;
      aCommandMapItem.HelpString = MyLoadString(IDS_CONTEXT_OPEN_HELP);
      aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, MyLoadString(IDS_CONTEXT_OPEN)); 
      m_CommandMap.push_back(aCommandMapItem);
      
      //////////////////////////
      // Extract command
      aCommandMapItem.CommandInternalID = kCommandInternalIDExtract;
      aCommandMapItem.Verb = kExtractVerb;
      aCommandMapItem.HelpString = MyLoadString(IDS_CONTEXT_EXTRACT_HELP);
      aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, MyLoadString(IDS_CONTEXT_EXTRACT)); 
      m_CommandMap.push_back(aCommandMapItem);
    }
  }

  if(m_FileNames.Size() > 0 && aCurrentCommandID <= aCommandIDLast)
  {
    aCommandMapItem.CommandInternalID = kCommandInternalIDCompress;
    aCommandMapItem.Verb = kCompressVerb;
    aCommandMapItem.HelpString = MyLoadString(IDS_CONTEXT_COMPRESS_HELP);
    aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, MyLoadString(IDS_CONTEXT_COMPRESS)); 
    m_CommandMap.push_back(aCommandMapItem);
  }


  CSysString aPopupMenuCaption = MyLoadString(IDS_CONTEXT_POPUP_CAPTION);

  // don't use InsertMenu:  See MSDN:
  // PRB: Duplicate Menu Items In the File Menu For a Shell Context Menu Extension
  // ID: Q214477 


  aMenuInfo.hSubMenu = aPopupMenu.Detach();
  aMenuInfo.dwTypeData = (LPTSTR)(LPCTSTR)aPopupMenuCaption;
  
  InsertMenuItem(hMenu, anIndexMenu++, TRUE, &aMenuInfo);

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, aCurrentCommandID - aCommandIDFirst); 
}


UINT CZipContextMenu::FindVerb(const CSysString &aVerb)
{
  for(int i = 0; i < m_CommandMap.size(); i++)
    if(m_CommandMap[i].Verb.Compare(aVerb) == 0)
      return i;
  return -1;
}

extern const char *kShellFolderClassIDString;

STDMETHODIMP CZipContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO aCommandInfo)
{
  int aCommandOffset;
  
  // commented for Windows 2000
  // /*

  #ifdef _UNICODE
  if(aCommandInfo->cbSize == sizeof(CMINVOKECOMMANDINFOEX) &&
    (aCommandInfo->fMask & CMIC_MASK_UNICODE) != 0)
  {
    LPCMINVOKECOMMANDINFOEX aCommandInfoEx = (LPCMINVOKECOMMANDINFOEX)aCommandInfo;
    if(HIWORD(aCommandInfoEx->lpVerb) == 0)
      aCommandOffset = LOWORD(aCommandInfoEx->lpVerb);
    else
      return E_FAIL;
    /*
    if(HIWORD(aCommandInfoEx->lpVerbW) == 0)
      aCommandOffset = LOWORD(aCommandInfoEx->lpVerbW);
    else
      aCommandOffset = FindVerb(GetSystemString(aCommandInfoEx->lpVerbW));
    */
  }
  else
  {
    return E_FAIL;
  }

  // else
  #else
  // */
  {
    if(HIWORD(aCommandInfo->lpVerb) == 0)
      aCommandOffset = LOWORD(aCommandInfo->lpVerb);
    else
      aCommandOffset = FindVerb(aCommandInfo->lpVerb);
  }
  #endif
  if(aCommandOffset < 0 || aCommandOffset >= m_CommandMap.size())
    return E_FAIL;

  ECommandInternalID aCommandInternalID = 
      m_CommandMap[aCommandOffset].CommandInternalID;
  HWND aHWND = aCommandInfo->hwnd;

  switch(aCommandInternalID)
  {
    case kCommandInternalIDOpen:
    {
      /*
      SHELLEXECUTEINFO anExecInfo;
      anExecInfo.cbSize = sizeof(anExecInfo);
      anExecInfo.fMask = SEE_MASK_CLASSNAME;
      anExecInfo.hwnd = NULL;
      anExecInfo.lpVerb = NULL;
      anExecInfo.lpFile = m_FileNames[0];
      anExecInfo.lpParameters = NULL;
      anExecInfo.lpDirectory = NULL;
      anExecInfo.nShow = SW_SHOWNORMAL;
      anExecInfo.lpClass = kFileClassIDString; // kShellFolderClassIDString;
      ::ShellExecuteEx(&anExecInfo);
      */
      CSysString aParams;
      if (!NSystem::MyGetWindowsDirectory(aParams))
        return E_FAIL;
      NFile::NName::NormalizeDirPathPrefix(aParams);
      aParams += _T("explorer.exe /e,/root,{23170F69-40C1-278A-1000-000100010000}, ");
      aParams += _T("\"");
      aParams += m_FileNames[0];
      aParams += _T("\"");

      WinExec(GetAnsiString(aParams), SW_SHOWNORMAL);
      /*
      aParams = "/e,/root,{23170F69-40C1-278A-1000-000100010000}, ";
      BOOL aResult = CreateProcess("c:\\windows\\explorer.exe", 
        (LPTSTR)(LPCTSTR)aParams, NULL, NULL, FALSE,
        NORMAL_PRIORITY_CLASS, NULL, NULL, NULL, NULL);
      */

      break;
    }
    case kCommandInternalIDExtract:
    {
      try
      {
        HRESULT aResult = ExtractArchive(aHWND, m_FileNames[0]);
        if (aResult == S_FALSE)
        {
          MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE);
        }
      }
      catch(...)
      {
        MyMessageBox("Error");
      }
      break;
    }
    case kCommandInternalIDCompress:
    {
      try
      {
        CompressArchive(m_FileNames);
      }
      catch(...)
      {
        MyMessageBox("Error");
      }
      break;
    }
  }
  return S_OK;
}

static void MyCopyString(void *aDestPointer, const TCHAR *aString, bool aWriteInUnicode)
{
  if(aWriteInUnicode)
    wcscpy((wchar_t *)aDestPointer, GetUnicodeString(aString));
  else
    strcpy((char *)aDestPointer, GetAnsiString(aString));
}

STDMETHODIMP CZipContextMenu::GetCommandString(UINT aCommandOffset, UINT uType, 
    UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
  switch(uType)
  { 
    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
      if(aCommandOffset < 0 || aCommandOffset >= (UINT)m_CommandMap.size())
        return S_FALSE;
      else 
        return S_OK;
  }
  if(aCommandOffset < 0 || aCommandOffset >= (UINT)m_CommandMap.size())
    return E_FAIL;
  if(uType == GCS_HELPTEXTA || uType == GCS_HELPTEXTW)
  {
    MyCopyString(pszName, m_CommandMap[aCommandOffset].HelpString,
        uType == GCS_HELPTEXTW);
    return NO_ERROR;
  }
  if(uType == GCS_VERBA || uType == GCS_VERBW)
  {
    MyCopyString(pszName, m_CommandMap[aCommandOffset].Verb,
        uType == GCS_VERBW);
    return NO_ERROR;
  }
  return E_FAIL;
}
