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
#include "Windows/Window.h"

#include "Windows/Menu.h"
#include "Windows/ResourceString.h"

#include "../../FileManager/ProgramLocation.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#include "resource.h"

#include "ExtractEngine.h"
#include "TestEngine.h"
#include "CompressEngine.h"
#include "MyMessages.h"

#include "../Resource/Extract/resource.h"

using namespace NWindows;

static LPCTSTR kFileClassIDString = _T("SevenZip");

///////////////////////////////
// IShellExtInit

HRESULT CZipContextMenu::GetFileNames(LPDATAOBJECT dataObject, 
    CSysStringVector &fileNames)
{
  fileNames.Clear();
  if(dataObject == NULL)
    return E_FAIL;
  FORMATETC fmte = {CF_HDROP,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  NCOM::CStgMedium aStgMedium;
  HRESULT result = dataObject->GetData(&fmte, &aStgMedium);
  if (result != S_OK)
    return result;
  aStgMedium._mustBeReleased = true;

  NShell::CDrop aDrop(false);
  NMemory::CGlobalLock aGlobalLock(aStgMedium->hGlobal);
  aDrop.Attach((HDROP)aGlobalLock.GetPointer());
  aDrop.QueryFileNames(fileNames);

  return S_OK;
}

STDMETHODIMP CZipContextMenu::Initialize(LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT dataObject, HKEY hkeyProgID)
{
  /*
  m_IsFolder = false;
  if (pidlFolder == 0)
  */
  // pidlFolder is NULL :(
  return GetFileNames(dataObject, _fileNames);
}

STDMETHODIMP CZipContextMenu::InitContextMenu(const wchar_t *folder, 
    const wchar_t **names, UINT32 numFiles)
{
  _fileNames.Clear();
  for (UINT32 i = 0; i < numFiles; i++)
    _fileNames.Add(GetSystemString(names[i]));
  return S_OK;
}


/////////////////////////////
// IContextMenu

static LPCTSTR kMainVerb = _T("SevenOpen");

static LPCTSTR kOpenVerb = _T("SevenOpen");
static LPCTSTR kExtractVerb = _T("SevenExtract");
static LPCTSTR kTestVerb = _T("SevenTest");
static LPCTSTR kCompressVerb = _T("SevenCompress");

STDMETHODIMP CZipContextMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu,
      UINT commandIDFirst, UINT commandIDLast, UINT flags)
{
  if(_fileNames.Size() == 0)
    return E_FAIL;
  UINT currentCommandID = commandIDFirst; 
  if ((flags & 0x000F) != CMF_NORMAL  &&
      (flags & CMF_VERBSONLY) == 0 &&
      (flags & CMF_EXPLORE) == 0) 
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID); 

  _commandMap.clear();
  CCommandMapItem commandMapItem;

  CMenu popupMenu;
  CMenuDestroyer menuDestroyer(popupMenu);

  if(!popupMenu.CreatePopup())
    throw 210503;

  commandMapItem.CommandInternalID = kCommandInternalNULL;
  commandMapItem.Verb = kMainVerb;
  commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_CAPTION_HELP, 0x02000102);
  _commandMap.push_back(commandMapItem);

  MENUITEMINFO menuItem;
  menuItem.cbSize = sizeof(menuItem);
  menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
  menuItem.fType = MFT_STRING;
  menuItem.wID = currentCommandID++; 

  int subMenuIndex = 0;
  if(_fileNames.Size() == 1 && currentCommandID <= commandIDLast)
  {
    const CSysString &aFileName = _fileNames.Front();
  
    NFile::NFind::CFileInfo fileInfo;
    if (!NFile::NFind::FindFile(_fileNames.Front(), fileInfo))
      return E_FAIL;
    if (!fileInfo.IsDirectory())
    {
      //////////////////////////
      // Open command
      commandMapItem.CommandInternalID = kCommandInternalIDOpen;
      commandMapItem.Verb = kOpenVerb;
      commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_OPEN_HELP, 0x02000104);
      popupMenu.AppendItem(MF_STRING, currentCommandID++, 
          LangLoadString(IDS_CONTEXT_OPEN, 0x02000103)); 
      _commandMap.push_back(commandMapItem);
      
      //////////////////////////
      // Extract command
      commandMapItem.CommandInternalID = kCommandInternalIDExtract;
      commandMapItem.Verb = kExtractVerb;
      commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_EXTRACT_HELP, 0x02000106);
      popupMenu.AppendItem(MF_STRING, currentCommandID++, 
          LangLoadString(IDS_CONTEXT_EXTRACT, 0x02000105)); 
      _commandMap.push_back(commandMapItem);

      //////////////////////////
      // Test command
      commandMapItem.CommandInternalID = kCommandInternalIDTest;
      commandMapItem.Verb = kTestVerb;
      commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_TEST_HELP, 0x0200010A);
      popupMenu.AppendItem(MF_STRING, currentCommandID++, 
          LangLoadString(IDS_CONTEXT_TEST, 0x02000109)); 
      _commandMap.push_back(commandMapItem);
    }
  }

  if(_fileNames.Size() > 0 && currentCommandID <= commandIDLast)
  {
    commandMapItem.CommandInternalID = kCommandInternalIDCompress;
    commandMapItem.Verb = kCompressVerb;
    commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_COMPRESS_HELP, 0x02000108);
    popupMenu.AppendItem(MF_STRING, currentCommandID++, 
        LangLoadString(IDS_CONTEXT_COMPRESS, 0x02000107)); 
    _commandMap.push_back(commandMapItem);
  }


  // CSysString aPopupMenuCaption = MyLoadString(IDS_CONTEXT_POPUP_CAPTION);
  CSysString aPopupMenuCaption = LangLoadString(IDS_CONTEXT_POPUP_CAPTION, 0x02000101);

  // don't use InsertMenu:  See MSDN:
  // PRB: Duplicate Menu Items In the File Menu For a Shell Context Menu Extension
  // ID: Q214477 


  menuItem.hSubMenu = popupMenu.Detach();
  menuDestroyer.Disable();
  menuItem.dwTypeData = (LPTSTR)(LPCTSTR)aPopupMenuCaption;
  
  InsertMenuItem(hMenu, indexMenu++, TRUE, &menuItem);

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID - commandIDFirst); 
}


UINT CZipContextMenu::FindVerb(const CSysString &verb)
{
  for(int i = 0; i < _commandMap.size(); i++)
    if(_commandMap[i].Verb.Compare(verb) == 0)
      return i;
  return -1;
}

extern const char *kShellFolderClassIDString;

/*
class CWindowDisable
{
  bool m_WasEnabled;
  CWindow m_Window;
public:
  CWindowDisable(HWND aWindow): m_Window(aWindow) 
  { 
    m_WasEnabled = m_Window.IsEnabled();
    if (m_WasEnabled)
      m_Window.Enable(false); 
  }
  ~CWindowDisable() 
  { 
    if (m_WasEnabled)
      m_Window.Enable(true); 
  }
};
*/

static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

static CSysString GetProgramCommand()
{
  CSysString path = TEXT("\"");
  CSysString folder;
  if (GetProgramFolderPath(folder))
    path += folder;
  if (IsItWindowsNT())
    path += TEXT("7zFMn.exe");
  else
    path += TEXT("7zFM.exe");
  path += TEXT("\"");
  // path += TEXT("\" \"%1\"");
  return path;
}

STDMETHODIMP CZipContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO commandInfo)
{
  int commandOffset;
  
  if(HIWORD(commandInfo->lpVerb) == 0)
    commandOffset = LOWORD(commandInfo->lpVerb);
  else
    commandOffset = FindVerb(GetSystemString(commandInfo->lpVerb));
  /*
  #ifdef _UNICODE
  if(commandInfo->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
  {
    if ((commandInfo->fMask & CMIC_MASK_UNICODE) != 0)
    {
      LPCMINVOKECOMMANDINFOEX aCommandInfoEx = (LPCMINVOKECOMMANDINFOEX)commandInfo;
      if(HIWORD(aCommandInfoEx->lpVerb) == 0)
        commandOffset = LOWORD(aCommandInfoEx->lpVerb);
      else
      {
        MessageBox(0, TEXT("1"), TEXT("1"), 0);
        return E_FAIL;
      }
    }
    else
    {
      if(HIWORD(commandInfo->lpVerb) == 0)
        commandOffset = LOWORD(commandInfo->lpVerb);
      else
        commandOffset = FindVerb(GetSystemString(commandInfo->lpVerb));
    }
    //  return E_FAIL;
  }
  else
  {
    if(HIWORD(commandInfo->lpVerb) == 0)
      commandOffset = LOWORD(commandInfo->lpVerb);
    else
      commandOffset = FindVerb(GetSystemString(commandInfo->lpVerb));
  }

  #else
  
  {
    if(HIWORD(commandInfo->lpVerb) == 0)
      commandOffset = LOWORD(commandInfo->lpVerb);
    else
      commandOffset = FindVerb(commandInfo->lpVerb);
  }

  #endif
  */

  if(commandOffset < 0 || commandOffset >= _commandMap.size())
    return E_FAIL;

  ECommandInternalID commandInternalID = 
      _commandMap[commandOffset].CommandInternalID;
  HWND aHWND = commandInfo->hwnd;

  // CWindowDisable aWindowDisable(aHWND);

  switch(commandInternalID)
  {
    case kCommandInternalIDOpen:
    {
      CSysString params;
      /*
      if (!NSystem::MyGetWindowsDirectory(params))
        return E_FAIL;
      NFile::NName::NormalizeDirPathPrefix(params);
      params += _T("7zFM.exe");
      */
      params = GetProgramCommand();
      params += _T(" \"");
      params += _fileNames[0];
      params += _T("\"");

      // WinExec(GetAnsiString(params), SW_SHOWNORMAL);

      STARTUPINFO startupInfo;
      startupInfo.cb = sizeof(startupInfo);
      startupInfo.lpReserved = 0;
      startupInfo.lpDesktop = 0;
      startupInfo.lpTitle = 0;
      startupInfo.dwFlags = 0;
      startupInfo.cbReserved2 = 0;
      startupInfo.lpReserved2 = 0;

      PROCESS_INFORMATION processInformation;
      BOOL result = CreateProcess(NULL, (TCHAR *)(const TCHAR *)params, 
            NULL, NULL, FALSE, 0, NULL, NULL, 
            &startupInfo, &processInformation);
      if (result != 0)
      {
        ::CloseHandle(processInformation.hThread);
        ::CloseHandle(processInformation.hProcess);
      }

      break;
    }
    case kCommandInternalIDExtract:
    {
      try
      {
        HRESULT result = ExtractArchive(aHWND, _fileNames[0]);
        if (result == S_FALSE)
          MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
        else if (result != S_OK)
          ShowErrorMessage(aHWND, result);
        return result;
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
        HRESULT result = TestArchive(aHWND, _fileNames[0]);
        if (result == S_FALSE)
          MyMessageBox(IDS_OPEN_IS_NOT_SUPORTED_ARCHIVE, 0x02000604);
        else if (result != S_OK)
          ShowErrorMessage(aHWND, result);
        return result;
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
        CompressArchive(_fileNames);
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

static void MyCopyString(void *destPointer, const TCHAR *string, bool writeInUnicode)
{
  if(writeInUnicode)
    wcscpy((wchar_t *)destPointer, GetUnicodeString(string));
  else
    strcpy((char *)destPointer, GetAnsiString(string));
}

STDMETHODIMP CZipContextMenu::GetCommandString(UINT commandOffset, UINT uType, 
    UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
  switch(uType)
  { 
    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
      if(commandOffset < 0 || commandOffset >= (UINT)_commandMap.size())
        return S_FALSE;
      else 
        return S_OK;
  }
  if(commandOffset < 0 || commandOffset >= (UINT)_commandMap.size())
    return E_FAIL;
  if(uType == GCS_HELPTEXTA || uType == GCS_HELPTEXTW)
  {
    MyCopyString(pszName, _commandMap[commandOffset].HelpString,
        uType == GCS_HELPTEXTW);
    return NO_ERROR;
  }
  if(uType == GCS_VERBA || uType == GCS_VERBW)
  {
    MyCopyString(pszName, _commandMap[commandOffset].Verb,
        uType == GCS_VERBW);
    return NO_ERROR;
  }
  return E_FAIL;
}
