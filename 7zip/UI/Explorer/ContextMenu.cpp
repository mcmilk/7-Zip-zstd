// ContextMenu.cpp

#include "StdAfx.h"

#include "ContextMenu.h"

#include "Common/StringConvert.h"

#include "Windows/Shell.h"
#include "Windows/Memory.h"
#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/System.h"
#include "Windows/Thread.h"
#include "Windows/Window.h"

#include "Windows/Menu.h"
#include "Windows/ResourceString.h"

#include "../../FileManager/FormatUtils.h"
#include "../../FileManager/ProgramLocation.h"

#include "../Common/ZipRegistry.h"
#include "../Common/ArchiveName.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

#include "resource.h"
#include "ContextMenuFlags.h"

// #include "ExtractEngine.h"
// #include "TestEngine.h"
// #include "CompressEngine.h"
#include "MyMessages.h"

#include "../Resource/Extract/resource.h"
#include "../Common/CompressCall.h"

using namespace NWindows;

static LPCTSTR kFileClassIDString = TEXT("SevenZip");

///////////////////////////////
// IShellExtInit

HRESULT CZipContextMenu::GetFileNames(LPDATAOBJECT dataObject, 
    CSysStringVector &fileNames)
{
  fileNames.Clear();
  if(dataObject == NULL)
    return E_FAIL;
  FORMATETC fmte = {CF_HDROP,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  NCOM::CStgMedium stgMedium;
  HRESULT result = dataObject->GetData(&fmte, &stgMedium);
  if (result != S_OK)
    return result;
  stgMedium._mustBeReleased = true;

  NShell::CDrop drop(false);
  NMemory::CGlobalLock globalLock(stgMedium->hGlobal);
  drop.Attach((HDROP)globalLock.GetPointer());
  drop.QueryFileNames(fileNames);

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

static LPCTSTR kMainVerb = TEXT("SevenZip");

/*
static LPCTSTR kOpenVerb = TEXT("SevenOpen");
static LPCTSTR kExtractVerb = TEXT("SevenExtract");
static LPCTSTR kExtractHereVerb = TEXT("SevenExtractHere");
static LPCTSTR kExtractToVerb = TEXT("SevenExtractTo");
static LPCTSTR kTestVerb = TEXT("SevenTest");
static LPCTSTR kCompressVerb = TEXT("SevenCompress");
static LPCTSTR kCompressToVerb = TEXT("SevenCompressTo");
static LPCTSTR kCompressEmailVerb = TEXT("SevenCompressEmail");
static LPCTSTR kCompressToEmailVerb = TEXT("SevenCompressToEmail");
*/

struct CContextMenuCommand
{
  UINT32 flag;
  CZipContextMenu::ECommandInternalID CommandInternalID;
  LPCTSTR Verb;
  UINT ResourceID;
  UINT ResourceHelpID;
  UINT32 LangID;
};

static CContextMenuCommand g_Commands[] = 
{
  { 
    NContextMenuFlags::kOpen,
    CZipContextMenu::kOpen, 
    TEXT("Open"), 
    IDS_CONTEXT_OPEN, 
    IDS_CONTEXT_OPEN_HELP, 
    0x02000103
  },
  { 
    NContextMenuFlags::kExtract, 
    CZipContextMenu::kExtract, 
    TEXT("Extract"), 
    IDS_CONTEXT_EXTRACT, 
    IDS_CONTEXT_EXTRACT_HELP, 
    0x02000105 
  },
  { 
    NContextMenuFlags::kExtractHere, 
    CZipContextMenu::kExtractHere, 
    TEXT("ExtractHere"), 
    IDS_CONTEXT_EXTRACT_HERE, 
    IDS_CONTEXT_EXTRACT_HERE_HELP, 
    0x0200010B
  },
  { 
    NContextMenuFlags::kExtractTo, 
    CZipContextMenu::kExtractTo, 
    TEXT("ExtractTo"), 
    IDS_CONTEXT_EXTRACT_TO, 
    IDS_CONTEXT_EXTRACT_TO_HELP, 
    0x0200010D
  },
  { 
    NContextMenuFlags::kTest, 
    CZipContextMenu::kTest, 
    TEXT("Test"), 
    IDS_CONTEXT_TEST, 
    IDS_CONTEXT_TEST_HELP, 
    0x02000109
  },
  { 
    NContextMenuFlags::kCompress, 
    CZipContextMenu::kCompress, 
    TEXT("Compress"), 
    IDS_CONTEXT_COMPRESS, 
    IDS_CONTEXT_COMPRESS_HELP, 
    0x02000107, 
  },
  { 
    NContextMenuFlags::kCompressTo, 
    CZipContextMenu::kCompressTo, 
    TEXT("CompressTo"), 
    IDS_CONTEXT_COMPRESS_TO, 
    IDS_CONTEXT_COMPRESS_TO_HELP, 
    0x0200010F
  },
  { 
    NContextMenuFlags::kCompressEmail, 
    CZipContextMenu::kCompressEmail, 
    TEXT("CompressEmail"), 
    IDS_CONTEXT_COMPRESS_EMAIL, 
    IDS_CONTEXT_COMPRESS_EMAIL_HELP, 
    0x02000111
  },
  { 
    NContextMenuFlags::kCompressToEmail, 
    CZipContextMenu::kCompressToEmail, 
    TEXT("CompressToEmail"), 
    IDS_CONTEXT_COMPRESS_TO_EMAIL, 
    IDS_CONTEXT_COMPRESS_TO_EMAIL_HELP, 
    0x02000113
  }
};

int FindCommand(CZipContextMenu::ECommandInternalID &id)
{
  for (int i = 0; i < sizeof(g_Commands) / sizeof(g_Commands[0]); i++)
    if (g_Commands[i].CommandInternalID == id)
      return i;
  return -1;
}

void CZipContextMenu::FillCommand(ECommandInternalID id, 
    CSysString &mainString, CCommandMapItem &commandMapItem)
{
  int i = FindCommand(id);
  if (i < 0)
    return;
  const CContextMenuCommand &command = g_Commands[i];
  commandMapItem.CommandInternalID = command.CommandInternalID;
  commandMapItem.Verb = command.Verb;
  commandMapItem.HelpString = LangLoadString(command.ResourceHelpID, command.LangID + 1);
  mainString = LangLoadString(command.ResourceID, command.LangID); 
}

void CZipContextMenu::FillCommand2(ECommandInternalID id, 
    UString &mainString, CCommandMapItem &commandMapItem)
{
  int i = FindCommand(id);
  if (i < 0)
    return;
  const CContextMenuCommand &command = g_Commands[i];
  commandMapItem.CommandInternalID = command.CommandInternalID;
  commandMapItem.Verb = command.Verb;
  commandMapItem.HelpString = LangLoadString(command.ResourceHelpID, command.LangID + 1);
  mainString = LangLoadStringW(command.ResourceID, command.LangID); 
}


/*
CSysString GetExtractPath(const CSysString &archiveName)
{
  CSysString s;
  int dotPos = s.ReverseFind('.');
  if (dotPos < 0)
    return archiveName;
  return archiveName.Left(dotPos);
}
*/

static BOOL MyInsertMenu(HMENU hMenu, int pos, UINT id, LPCTSTR s)
{
  MENUITEMINFO menuItem;
  menuItem.cbSize = sizeof(menuItem);
  menuItem.fType = MFT_STRING;
  menuItem.fMask = MIIM_TYPE | MIIM_ID;
  menuItem.wID = id; 
  menuItem.dwTypeData = (LPTSTR)(LPCTSTR)s;
  return ::InsertMenuItem(hMenu, pos++, TRUE, &menuItem);
}

static CSysString GetSubFolderNameForExtract(const CSysString &archiveName)
{
  int dotPos = archiveName.ReverseFind('.');
  if (dotPos >= 0)
    return archiveName.Left(dotPos);
  return archiveName + CSysString(TEXT("~"));
}

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

  CMenu popupMenu;
  CMenuDestroyer menuDestroyer;

  bool cascadedMenu = ReadCascadedMenu();
  MENUITEMINFO menuItem;
  UINT subIndex = indexMenu;
  if (cascadedMenu)
  {
    CCommandMapItem commandMapItem;
    if(!popupMenu.CreatePopup())
      throw 210503;
    menuDestroyer.Attach(popupMenu);
    commandMapItem.CommandInternalID = kCommandNULL;
    commandMapItem.Verb = kMainVerb;
    commandMapItem.HelpString = LangLoadString(IDS_CONTEXT_CAPTION_HELP, 0x02000102);
    _commandMap.push_back(commandMapItem);
    
    menuItem.wID = currentCommandID++; 
    subIndex = 0;
  }
  else
  {
    popupMenu.Attach(hMenu);
  }

  UINT32 contextMenuFlags;
  if (!ReadContextMenuStatus(contextMenuFlags))
    contextMenuFlags = NContextMenuFlags::GetDefaultFlags();

  int subMenuIndex = 0;
  CSysString mainString;
  if(_fileNames.Size() == 1 && currentCommandID + 6 <= commandIDLast)
  {
    const CSysString &fileName = _fileNames.Front();

    CSysString folderPrefix;
    NFile::NDirectory::GetOnlyDirPrefix(fileName, folderPrefix);
   
    NFile::NFind::CFileInfo fileInfo;
    if (!NFile::NFind::FindFile(fileName, fileInfo))
      return E_FAIL;
    if (!fileInfo.IsDirectory())
    {
      // Open
      if ((contextMenuFlags & NContextMenuFlags::kOpen) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kOpen, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.push_back(commandMapItem);
      }
      
      // Extract
      if ((contextMenuFlags & NContextMenuFlags::kExtract) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kExtract, mainString, commandMapItem);
        commandMapItem.Folder = folderPrefix + 
            GetSubFolderNameForExtract(fileInfo.Name) + 
            CSysString(L'\\');
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.push_back(commandMapItem);
      }

      // Extract Here
      if ((contextMenuFlags & NContextMenuFlags::kExtractHere) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kExtractHere, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        commandMapItem.Folder = folderPrefix;
        _commandMap.push_back(commandMapItem);
      }

      // Extract To
      if ((contextMenuFlags & NContextMenuFlags::kExtractTo) != 0)
      {
        CCommandMapItem commandMapItem;
        UString s;
        FillCommand2(kExtractTo, s, commandMapItem);
        CSysString folder = GetSubFolderNameForExtract(fileInfo.Name) + 
            CSysString(L'\\');
        commandMapItem.Folder = folderPrefix + folder;
        s = MyFormatNew(s, GetUnicodeString(folder));
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, GetSystemString(s)); 
        _commandMap.push_back(commandMapItem);
      }
      
      // Test
      if ((contextMenuFlags & NContextMenuFlags::kTest) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kTest, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.push_back(commandMapItem);
      }
    }
  }

  if(_fileNames.Size() > 0 && currentCommandID + 6 <= commandIDLast)
  {
    const CSysString &fileName = _fileNames.Front();
    CSysString archiveName = CreateArchiveName(fileName, _fileNames.Size() > 1, false);
    CSysString archiveName7z = archiveName + TEXT(".7z");
    CSysString archivePathPrefix;
    NFile::NDirectory::GetOnlyDirPrefix(fileName, archivePathPrefix);

    // Compress
    if ((contextMenuFlags & NContextMenuFlags::kCompress) != 0)
    {
      CCommandMapItem commandMapItem;
      commandMapItem.Archive = archivePathPrefix + archiveName;
      FillCommand(kCompress, mainString, commandMapItem);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
      _commandMap.push_back(commandMapItem);
    }

    
    // CompressTo
    if (contextMenuFlags & NContextMenuFlags::kCompressTo)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand2(kCompressTo, s, commandMapItem);
      commandMapItem.Archive = archivePathPrefix + archiveName7z;
      UString t = UString(L"\"") + GetUnicodeString(archiveName7z) + UString(L"\"");
      s = MyFormatNew(s, GetUnicodeString(t));
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, GetSystemString(s)); 
      _commandMap.push_back(commandMapItem);
    }

    // CompressEmail
    if ((contextMenuFlags & NContextMenuFlags::kCompressEmail) != 0)
    {
      CCommandMapItem commandMapItem;
      commandMapItem.Archive = archiveName;
      FillCommand(kCompressEmail, mainString, commandMapItem);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
      _commandMap.push_back(commandMapItem);
    }

    // CompressToEmail
    if ((contextMenuFlags & NContextMenuFlags::kCompressToEmail) != 0)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand2(kCompressToEmail, s, commandMapItem);
      commandMapItem.Archive = archiveName7z;
      UString t = UString(L"\"") + GetUnicodeString(archiveName7z) + UString(L"\"");
      s = MyFormatNew(s, GetUnicodeString(t));
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, GetSystemString(s)); 
      _commandMap.push_back(commandMapItem);
    }
  }


  CSysString popupMenuCaption = LangLoadString(IDS_CONTEXT_POPUP_CAPTION, 0x02000101);

  // don't use InsertMenu:  See MSDN:
  // PRB: Duplicate Menu Items In the File Menu For a Shell Context Menu Extension
  // ID: Q214477 

  if (cascadedMenu)
  {
    MENUITEMINFO menuItem;
    menuItem.cbSize = sizeof(menuItem);
    menuItem.fType = MFT_STRING;
    menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
    menuItem.wID = currentCommandID++; 
    menuItem.hSubMenu = popupMenu.Detach();
    menuDestroyer.Disable();
    menuItem.dwTypeData = (LPTSTR)(LPCTSTR)popupMenuCaption;
    ::InsertMenuItem(hMenu, indexMenu++, TRUE, &menuItem);
  }

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

/*
struct CThreadCompressMain
{
  CSysStringVector FileNames;

  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    try
    {
      HRESULT result = CompressArchive(FileNames);
    }
    catch(...)
    {
      MyMessageBox(IDS_ERROR, 0x02000605);
    }
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    CThreadCompressMain *compressor = (CThreadCompressMain *)param;
    return ((CThreadCompressMain *)param)->Process();
    delete compressor;
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

  const CCommandMapItem commandMapItem = _commandMap[commandOffset];
  ECommandInternalID commandInternalID = commandMapItem.CommandInternalID;
  HWND aHWND = commandInfo->hwnd;

  // CWindowDisable aWindowDisable(aHWND);

  try
  {
    switch(commandInternalID)
    {
      case kOpen:
      {
        CSysString params;
        params = GetProgramCommand();
        params += TEXT(" \"");
        params += _fileNames[0];
        params += TEXT("\"");
        MyCreateProcess(params);
        break;
      }
      case kExtract:
      case kExtractHere:
      case kExtractTo:
      {
        ExtractArchive(_fileNames[0], commandMapItem.Folder,
            (commandInternalID == kExtract));
        break;
      }
      case kTest:
      {
        TestArchive(_fileNames[0]);
        break;
      }
      case kCompress:
      case kCompressTo:
      case kCompressEmail:
      case kCompressToEmail:
      {
        bool email = (commandInternalID == kCompressEmail) || 
          (commandInternalID == kCompressToEmail);
        bool showDialog = (commandInternalID == kCompress) || 
          (commandInternalID == kCompressEmail);
        UStringVector fileNamesUnicode;
        for (int i = 0; i < _fileNames.Size(); i++)
          fileNamesUnicode.Add(GetUnicodeString(_fileNames[i]));
        CompressFiles(commandMapItem.Archive, fileNamesUnicode, email, showDialog);
        break;
      }
    }
  }
  catch(...)
  {
    MyMessageBox(IDS_ERROR, 0x02000605);
  }
  return S_OK;
}

static void MyCopyString(void *destPointer, const TCHAR *string, bool writeInUnicode)
{
  if(writeInUnicode)
  {
    wcscpy((wchar_t *)destPointer, GetUnicodeString(string));
  }
  else
    lstrcpyA((char *)destPointer, GetAnsiString(string));
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
