// ContextMenu.cpp

#include "StdAfx.h"

#include "ContextMenu.h"

#include "Common/StringConvert.h"

#include "Windows/Shell.h"
#include "Windows/Memory.h"
#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/System.h"

#include "Windows/Menu.h"
#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

#include "resource.h"

#include "ExtractEngine.h"
#include "TestEngine.h"
#include "CompressEngine.h"
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

static LPCTSTR kOpenVerb = _T("SevenOpen");
static LPCTSTR kExtractVerb = _T("SevenExtract");
static LPCTSTR kTestVerb = _T("SevenTest");
static LPCTSTR kCompressVerb = _T("SevenCompress");

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
  aCommandMapItem.HelpString = LangLoadString(IDS_CONTEXT_CAPTION_HELP, 0x02000102);
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
      aCommandMapItem.HelpString = LangLoadString(IDS_CONTEXT_OPEN_HELP, 0x02000104);
      aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, 
          LangLoadString(IDS_CONTEXT_OPEN, 0x02000103)); 
      m_CommandMap.push_back(aCommandMapItem);
      
      //////////////////////////
      // Extract command
      aCommandMapItem.CommandInternalID = kCommandInternalIDExtract;
      aCommandMapItem.Verb = kExtractVerb;
      aCommandMapItem.HelpString = LangLoadString(IDS_CONTEXT_EXTRACT_HELP, 0x02000106);
      aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, 
          LangLoadString(IDS_CONTEXT_EXTRACT, 0x02000105)); 
      m_CommandMap.push_back(aCommandMapItem);

      //////////////////////////
      // Test command
      aCommandMapItem.CommandInternalID = kCommandInternalIDTest;
      aCommandMapItem.Verb = kTestVerb;
      aCommandMapItem.HelpString = LangLoadString(IDS_CONTEXT_TEST_HELP, 0x0200010A);
      aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, 
          LangLoadString(IDS_CONTEXT_TEST, 0x02000109)); 
      m_CommandMap.push_back(aCommandMapItem);
    }
  }

  if(m_FileNames.Size() > 0 && aCurrentCommandID <= aCommandIDLast)
  {
    aCommandMapItem.CommandInternalID = kCommandInternalIDCompress;
    aCommandMapItem.Verb = kCompressVerb;
    aCommandMapItem.HelpString = LangLoadString(IDS_CONTEXT_COMPRESS_HELP, 0x02000108);
    aPopupMenu.AppendItem(MF_STRING, aCurrentCommandID++, 
        LangLoadString(IDS_CONTEXT_COMPRESS, 0x02000107)); 
    m_CommandMap.push_back(aCommandMapItem);
  }


  // CSysString aPopupMenuCaption = MyLoadString(IDS_CONTEXT_POPUP_CAPTION);
  CSysString aPopupMenuCaption = LangLoadString(IDS_CONTEXT_POPUP_CAPTION, 0x02000101);

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
  
  if(HIWORD(aCommandInfo->lpVerb) == 0)
    aCommandOffset = LOWORD(aCommandInfo->lpVerb);
  else
    aCommandOffset = FindVerb(GetSystemString(aCommandInfo->lpVerb));
  /*
  #ifdef _UNICODE
  if(aCommandInfo->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
  {
    if ((aCommandInfo->fMask & CMIC_MASK_UNICODE) != 0)
    {
      LPCMINVOKECOMMANDINFOEX aCommandInfoEx = (LPCMINVOKECOMMANDINFOEX)aCommandInfo;
      if(HIWORD(aCommandInfoEx->lpVerb) == 0)
        aCommandOffset = LOWORD(aCommandInfoEx->lpVerb);
      else
      {
        MessageBox(0, TEXT("1"), TEXT("1"), 0);
        return E_FAIL;
      }
    }
    else
    {
      if(HIWORD(aCommandInfo->lpVerb) == 0)
        aCommandOffset = LOWORD(aCommandInfo->lpVerb);
      else
        aCommandOffset = FindVerb(GetSystemString(aCommandInfo->lpVerb));
    }
    //  return E_FAIL;
  }
  else
  {
    if(HIWORD(aCommandInfo->lpVerb) == 0)
      aCommandOffset = LOWORD(aCommandInfo->lpVerb);
    else
      aCommandOffset = FindVerb(GetSystemString(aCommandInfo->lpVerb));
  }

  #else
  
  {
    if(HIWORD(aCommandInfo->lpVerb) == 0)
      aCommandOffset = LOWORD(aCommandInfo->lpVerb);
    else
      aCommandOffset = FindVerb(aCommandInfo->lpVerb);
  }

  #endif
  */

  if(aCommandOffset < 0 || aCommandOffset >= m_CommandMap.size())
    return E_FAIL;

  ECommandInternalID aCommandInternalID = 
      m_CommandMap[aCommandOffset].CommandInternalID;
  HWND aHWND = aCommandInfo->hwnd;

  switch(aCommandInternalID)
  {
    case kCommandInternalIDOpen:
    {
      CSysString aParams;
      if (!NSystem::MyGetWindowsDirectory(aParams))
        return E_FAIL;
      NFile::NName::NormalizeDirPathPrefix(aParams);
      aParams += _T("explorer.exe /e,/root,{23170F69-40C1-278A-1000-000100010000}, ");
      aParams += _T("\"");
      aParams += m_FileNames[0];
      aParams += _T("\"");

      // WinExec(GetAnsiString(aParams), SW_SHOWNORMAL);

      STARTUPINFO aStartupInfo;
      aStartupInfo.cb = sizeof(aStartupInfo);
      aStartupInfo.lpReserved = 0;
      aStartupInfo.lpDesktop = 0;
      aStartupInfo.lpTitle = 0;
      aStartupInfo.dwFlags = 0;
      aStartupInfo.cbReserved2 = 0;
      aStartupInfo.lpReserved2 = 0;

      PROCESS_INFORMATION aProcessInformation;
      BOOL aResult = CreateProcess(NULL, (TCHAR *)(const TCHAR *)aParams, 
            NULL, NULL, FALSE, 0, NULL, NULL, 
            &aStartupInfo, &aProcessInformation);
      ::CloseHandle(aProcessInformation.hProcess);

      break;
    }
    case kCommandInternalIDExtract:
    {
      try
      {
        if (ExtractArchive(aHWND, m_FileNames[0]) == S_FALSE)
          MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      }
      catch(...)
      {
        MyMessageBox(IDS_ERROR, 0x02000605);
      }
      break;
    }
    case kCommandInternalIDTest:
    {
      try
      {
        if (TestArchive(aHWND, m_FileNames[0]) == S_FALSE)
          MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
      }
      catch(...)
      {
        MyMessageBox(IDS_ERROR, 0x02000605);
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
        MyMessageBox(IDS_ERROR, 0x02000605);
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
