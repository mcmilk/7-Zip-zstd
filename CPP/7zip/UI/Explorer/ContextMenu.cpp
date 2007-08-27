// ContextMenu.cpp

#include "StdAfx.h"

#include "ContextMenu.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "Windows/Shell.h"
#include "Windows/Memory.h"
#include "Windows/COM.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Thread.h"
#include "Windows/Window.h"

#include "Windows/Menu.h"
#include "Windows/ResourceString.h"

#include "../FileManager/FormatUtils.h"
#include "../FileManager/ProgramLocation.h"

#include "../Common/ZipRegistry.h"
#include "../Common/ArchiveName.h"

#ifdef LANG        
#include "../FileManager/LangUtils.h"
#endif

#include "resource.h"
#include "ContextMenuFlags.h"

// #include "ExtractEngine.h"
// #include "TestEngine.h"
// #include "CompressEngine.h"
#include "MyMessages.h"

#include "../GUI/ExtractRes.h"
#include "../Common/CompressCall.h"

using namespace NWindows;

///////////////////////////////
// IShellExtInit

extern LONG g_DllRefCount;
    
CZipContextMenu::CZipContextMenu()  { InterlockedIncrement(&g_DllRefCount); }
CZipContextMenu::~CZipContextMenu() { InterlockedDecrement(&g_DllRefCount); }

HRESULT CZipContextMenu::GetFileNames(LPDATAOBJECT dataObject, UStringVector &fileNames)
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
    LPDATAOBJECT dataObject, HKEY /* hkeyProgID */)
{
  // OutputDebugString(TEXT("::Initialize\r\n"));
  _dropMode = false;
  _dropPath.Empty();
  if (pidlFolder != 0)
  {
    if (NShell::GetPathFromIDList(pidlFolder, _dropPath))
    {
      // OutputDebugString(path);
      // OutputDebugString(TEXT("\r\n"));
      NFile::NName::NormalizeDirPathPrefix(_dropPath);
      _dropMode = !_dropPath.IsEmpty();
    }
    else
      _dropPath.Empty();
  }

  /*
  m_IsFolder = false;
  if (pidlFolder == 0)
  */
  // pidlFolder is NULL :(
  return GetFileNames(dataObject, _fileNames);
}

STDMETHODIMP CZipContextMenu::InitContextMenu(const wchar_t * /* folder */, 
    const wchar_t **names, UINT32 numFiles)
{
  _fileNames.Clear();
  for (UINT32 i = 0; i < numFiles; i++)
    _fileNames.Add(names[i]);
  _dropMode = false;
  return S_OK;
}


/////////////////////////////
// IContextMenu

static LPCWSTR kMainVerb = L"SevenZip";

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
  LPCWSTR Verb;
  UINT ResourceID;
  UINT ResourceHelpID;
  UINT32 LangID;
};

static CContextMenuCommand g_Commands[] = 
{
  { 
    NContextMenuFlags::kOpen,
    CZipContextMenu::kOpen, 
    L"Open", 
    IDS_CONTEXT_OPEN, 
    IDS_CONTEXT_OPEN_HELP, 
    0x02000103
  },
  { 
    NContextMenuFlags::kExtract, 
    CZipContextMenu::kExtract, 
    L"Extract", 
    IDS_CONTEXT_EXTRACT, 
    IDS_CONTEXT_EXTRACT_HELP, 
    0x02000105 
  },
  { 
    NContextMenuFlags::kExtractHere, 
    CZipContextMenu::kExtractHere, 
    L"ExtractHere", 
    IDS_CONTEXT_EXTRACT_HERE, 
    IDS_CONTEXT_EXTRACT_HERE_HELP, 
    0x0200010B
  },
  { 
    NContextMenuFlags::kExtractTo, 
    CZipContextMenu::kExtractTo, 
    L"ExtractTo", 
    IDS_CONTEXT_EXTRACT_TO, 
    IDS_CONTEXT_EXTRACT_TO_HELP, 
    0x0200010D
  },
  { 
    NContextMenuFlags::kTest, 
    CZipContextMenu::kTest, 
    L"Test", 
    IDS_CONTEXT_TEST, 
    IDS_CONTEXT_TEST_HELP, 
    0x02000109
  },
  { 
    NContextMenuFlags::kCompress, 
    CZipContextMenu::kCompress, 
    L"Compress", 
    IDS_CONTEXT_COMPRESS, 
    IDS_CONTEXT_COMPRESS_HELP, 
    0x02000107, 
  },
  { 
    NContextMenuFlags::kCompressEmail, 
    CZipContextMenu::kCompressEmail, 
    L"CompressEmail", 
    IDS_CONTEXT_COMPRESS_EMAIL, 
    IDS_CONTEXT_COMPRESS_EMAIL_HELP, 
    0x02000111
  },
  { 
    NContextMenuFlags::kCompressTo7z, 
    CZipContextMenu::kCompressTo7z, 
    L"CompressTo7z", 
    IDS_CONTEXT_COMPRESS_TO, 
    IDS_CONTEXT_COMPRESS_TO_HELP, 
    0x0200010F
  },
  { 
    NContextMenuFlags::kCompressTo7zEmail, 
    CZipContextMenu::kCompressTo7zEmail, 
    L"CompressTo7zEmail", 
    IDS_CONTEXT_COMPRESS_TO_EMAIL, 
    IDS_CONTEXT_COMPRESS_TO_EMAIL_HELP, 
    0x02000113
  },
  { 
    NContextMenuFlags::kCompressToZip, 
    CZipContextMenu::kCompressToZip, 
    L"CompressToZip", 
    IDS_CONTEXT_COMPRESS_TO, 
    IDS_CONTEXT_COMPRESS_TO_HELP, 
    0x0200010F
  },
  { 
    NContextMenuFlags::kCompressToZipEmail, 
    CZipContextMenu::kCompressToZipEmail, 
    L"CompressToZipEmail", 
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
    UString &mainString, CCommandMapItem &commandMapItem)
{
  int i = FindCommand(id);
  if (i < 0)
    return;
  const CContextMenuCommand &command = g_Commands[i];
  commandMapItem.CommandInternalID = command.CommandInternalID;
  commandMapItem.Verb = (UString)kMainVerb + (UString)command.Verb;
  commandMapItem.HelpString = LangString(command.ResourceHelpID, command.LangID + 1);
  mainString = LangString(command.ResourceID, command.LangID); 
}

static bool MyInsertMenu(CMenu &menu, int pos, UINT id, const UString &s)
{
  CMenuItem menuItem;
  menuItem.fType = MFT_STRING;
  menuItem.fMask = MIIM_TYPE | MIIM_ID;
  menuItem.wID = id; 
  menuItem.StringValue = s;
  return menu.InsertItem(pos, true, menuItem);
}

static const wchar_t *kArcExts[] = 
{
  L"7z",
  L"bz2",
  L"gz",
  L"rar",
  L"zip"
};

static bool IsItArcExt(const UString &ext2)
{
  UString ext = ext2;
  ext.MakeLower();
  for (int i = 0; i < sizeof(kArcExts) / sizeof(kArcExts[0]); i++)
    if (ext.Compare(kArcExts[i]) == 0)
      return true;
  return false;
}

static UString GetSubFolderNameForExtract(const UString &archiveName)
{
  int dotPos = archiveName.ReverseFind(L'.');
  if (dotPos < 0)
    return archiveName + UString(L"~");
  const UString ext = archiveName.Mid(dotPos + 1);
  UString res = archiveName.Left(dotPos);
  res.TrimRight();
  dotPos = res.ReverseFind(L'.');
  if (dotPos > 0)
  {
    const UString ext2 = res.Mid(dotPos + 1);
    if (ext.CompareNoCase(L"rar") == 0 && 
        (ext2.CompareNoCase(L"part001") == 0 || 
         ext2.CompareNoCase(L"part01") == 0 || 
         ext2.CompareNoCase(L"part1") == 0) ||
        IsItArcExt(ext2) && ext.CompareNoCase(L"001") == 0)
      res = res.Left(dotPos);
    res.TrimRight();
  }
  return res;
}

static UString GetReducedString(const UString &s)
{
  const int kMaxSize = 64;
  if (s.Length() < kMaxSize)
    return s;
  const int kFirstPartSize = kMaxSize / 2;
  return s.Left(kFirstPartSize) + UString(L" ... ") + s.Right(kMaxSize - kFirstPartSize);
}

static const wchar_t *kExtractExludeExtensions[] = 
{
  L"txt", L"htm", L"html", L"xml",
  L"bmp", L"gif", L"jpeg", L"jpg"
};

static bool DoNeedExtract(const UString &name)
{
  int extPos = name.ReverseFind('.');
  if (extPos < 0)
    return true;
  UString ext = name.Mid(extPos + 1);
  ext.MakeLower();
  for (int i = 0; i < sizeof(kExtractExludeExtensions) / sizeof(kExtractExludeExtensions[0]); i++)
    if (ext.Compare(kExtractExludeExtensions[i]) == 0)
      return false;
  return true;
}

STDMETHODIMP CZipContextMenu::QueryContextMenu(HMENU hMenu, UINT indexMenu,
      UINT commandIDFirst, UINT commandIDLast, UINT flags)
{
  LoadLangOneTime();
  if(_fileNames.Size() == 0)
    return E_FAIL;
  UINT currentCommandID = commandIDFirst; 
  if ((flags & 0x000F) != CMF_NORMAL  &&
      (flags & CMF_VERBSONLY) == 0 &&
      (flags & CMF_EXPLORE) == 0) 
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID); 

  _commandMap.Clear();

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
    commandMapItem.HelpString = LangString(IDS_CONTEXT_CAPTION_HELP, 0x02000102);
    _commandMap.Add(commandMapItem);
    
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

  UString mainString;
  if(_fileNames.Size() == 1 && currentCommandID + 6 <= commandIDLast)
  {
    const UString &fileName = _fileNames.Front();
    UString folderPrefix;
    NFile::NDirectory::GetOnlyDirPrefix(fileName, folderPrefix);
   
    NFile::NFind::CFileInfoW fileInfo;
    if (!NFile::NFind::FindFile(fileName, fileInfo))
      return E_FAIL;
    if (!fileInfo.IsDirectory() && DoNeedExtract(fileInfo.Name))
    {
      // Open
      if ((contextMenuFlags & NContextMenuFlags::kOpen) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kOpen, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.Add(commandMapItem);
      }
    }
  }

  if(_fileNames.Size() > 0 && currentCommandID + 10 <= commandIDLast)
  {
    bool needExtract = false;
    for(int i = 0; i < _fileNames.Size(); i++)
    {
      NFile::NFind::CFileInfoW fileInfo;
      if (!NFile::NFind::FindFile(_fileNames[i], fileInfo))
        return E_FAIL;
      if (!fileInfo.IsDirectory() && DoNeedExtract(fileInfo.Name))
        needExtract = true;
    }
    const UString &fileName = _fileNames.Front();
    if (needExtract)
    {
      UString folderPrefix;
      NFile::NDirectory::GetOnlyDirPrefix(fileName, folderPrefix);
      NFile::NFind::CFileInfoW fileInfo;
      if (!NFile::NFind::FindFile(fileName, fileInfo))
        return E_FAIL;
      // Extract
      if ((contextMenuFlags & NContextMenuFlags::kExtract) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kExtract, mainString, commandMapItem);
        if (_dropMode)
          commandMapItem.Folder = _dropPath;
        else
          commandMapItem.Folder = folderPrefix;
        commandMapItem.Folder += GetSubFolderNameForExtract(fileInfo.Name) + UString(L'\\');
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.Add(commandMapItem);
      }

      // Extract Here
      if ((contextMenuFlags & NContextMenuFlags::kExtractHere) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kExtractHere, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        if (_dropMode)
          commandMapItem.Folder = _dropPath;
        else
          commandMapItem.Folder = folderPrefix;
        _commandMap.Add(commandMapItem);
      }

      // Extract To
      if ((contextMenuFlags & NContextMenuFlags::kExtractTo) != 0)
      {
        CCommandMapItem commandMapItem;
        UString s;
        FillCommand(kExtractTo, s, commandMapItem);
        UString folder;
        if (_fileNames.Size() == 1)
          folder = GetSubFolderNameForExtract(fileInfo.Name);
        else
          folder = L'*'; 
        if (_dropMode)
          commandMapItem.Folder = _dropPath;
        else
          commandMapItem.Folder = folderPrefix;
        commandMapItem.Folder += folder;
        s = MyFormatNew(s, GetReducedString(UString(L"\"") + folder + UString(L"\\\"")));
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s); 
        _commandMap.Add(commandMapItem);
      }
      // Test
      if ((contextMenuFlags & NContextMenuFlags::kTest) != 0)
      {
        CCommandMapItem commandMapItem;
        FillCommand(kTest, mainString, commandMapItem);
        MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
        _commandMap.Add(commandMapItem);
      }
    }
    UString archiveName = CreateArchiveName(fileName, _fileNames.Size() > 1, false);
    UString archiveName7z = archiveName + L".7z";
    UString archiveNameZip = archiveName + L".zip";
    UString archivePathPrefix;
    NFile::NDirectory::GetOnlyDirPrefix(fileName, archivePathPrefix);

    // Compress
    if ((contextMenuFlags & NContextMenuFlags::kCompress) != 0)
    {
      CCommandMapItem commandMapItem;
      if (_dropMode)
        commandMapItem.Folder = _dropPath;
      else
        commandMapItem.Folder = archivePathPrefix;
      commandMapItem.Archive = archiveName;
      FillCommand(kCompress, mainString, commandMapItem);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
      _commandMap.Add(commandMapItem);
    }

    
    // CompressEmail
    if ((contextMenuFlags & NContextMenuFlags::kCompressEmail) != 0 && !_dropMode)
    {
      CCommandMapItem commandMapItem;
      commandMapItem.Archive = archiveName;
      FillCommand(kCompressEmail, mainString, commandMapItem);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, mainString); 
      _commandMap.Add(commandMapItem);
    }

    // CompressTo7z
    if (contextMenuFlags & NContextMenuFlags::kCompressTo7z)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand(kCompressTo7z, s, commandMapItem);
      if (_dropMode)
        commandMapItem.Folder = _dropPath;
      else
        commandMapItem.Folder = archivePathPrefix;
      commandMapItem.Archive = archiveName7z;
      commandMapItem.ArchiveType = L"7z";
      UString t = UString(L"\"") + GetReducedString(archiveName7z) + UString(L"\"");
      s = MyFormatNew(s, t);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s); 
      _commandMap.Add(commandMapItem);
    }

    // CompressTo7zEmail
    if ((contextMenuFlags & NContextMenuFlags::kCompressTo7zEmail) != 0  && !_dropMode)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand(kCompressTo7zEmail, s, commandMapItem);
      commandMapItem.Archive = archiveName7z;
      commandMapItem.ArchiveType = L"7z";
      UString t = UString(L"\"") + GetReducedString(archiveName7z) + UString(L"\"");
      s = MyFormatNew(s, t);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s); 
      _commandMap.Add(commandMapItem);
    }

    // CompressToZip
    if (contextMenuFlags & NContextMenuFlags::kCompressToZip)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand(kCompressToZip, s, commandMapItem);
      if (_dropMode)
        commandMapItem.Folder = _dropPath;
      else
        commandMapItem.Folder = archivePathPrefix;
      commandMapItem.Archive = archiveNameZip;
      commandMapItem.ArchiveType = L"zip";
      UString t = UString(L"\"") + GetReducedString(archiveNameZip) + UString(L"\"");
      s = MyFormatNew(s, t);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s); 
      _commandMap.Add(commandMapItem);
    }

    // CompressToZipEmail
    if ((contextMenuFlags & NContextMenuFlags::kCompressToZipEmail) != 0  && !_dropMode)
    {
      CCommandMapItem commandMapItem;
      UString s;
      FillCommand(kCompressToZipEmail, s, commandMapItem);
      commandMapItem.Archive = archiveNameZip;
      commandMapItem.ArchiveType = L"zip";
      UString t = UString(L"\"") + GetReducedString(archiveNameZip) + UString(L"\"");
      s = MyFormatNew(s, t);
      MyInsertMenu(popupMenu, subIndex++, currentCommandID++, s); 
      _commandMap.Add(commandMapItem);
    }
  }


  // don't use InsertMenu:  See MSDN:
  // PRB: Duplicate Menu Items In the File Menu For a Shell Context Menu Extension
  // ID: Q214477 

  if (cascadedMenu)
  {
    CMenuItem menuItem;
    menuItem.fType = MFT_STRING;
    menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
    menuItem.wID = currentCommandID++; 
    menuItem.hSubMenu = popupMenu.Detach();
    menuDestroyer.Disable();
    menuItem.StringValue = LangString(IDS_CONTEXT_POPUP_CAPTION, 0x02000101);
    CMenu menu;
    menu.Attach(hMenu);
    menu.InsertItem(indexMenu++, true, menuItem);
  }

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID - commandIDFirst); 
}


int CZipContextMenu::FindVerb(const UString &verb)
{
  for(int i = 0; i < _commandMap.Size(); i++)
    if(_commandMap[i].Verb.Compare(verb) == 0)
      return i;
  return -1;
}

extern const char *kShellFolderClassIDString;


static UString GetProgramCommand()
{
  UString path = L"\"";
  UString folder;
  if (GetProgramFolderPath(folder))
    path += folder;
  path += L"7zFM.exe\"";
  return path;
}

STDMETHODIMP CZipContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO commandInfo)
{
  // ::OutputDebugStringA("1");
  int commandOffset;

  // It's fix for bug: crashing in XP. See example in MSDN: "Creating Context Menu Handlers".

  if (commandInfo->cbSize == sizeof(CMINVOKECOMMANDINFOEX) &&
      (commandInfo->fMask & CMIC_MASK_UNICODE) != 0)
  {
    LPCMINVOKECOMMANDINFOEX commandInfoEx = (LPCMINVOKECOMMANDINFOEX)commandInfo;
    if(HIWORD(commandInfoEx->lpVerbW) == 0)
      commandOffset = LOWORD(commandInfo->lpVerb);
    else
      commandOffset = FindVerb(commandInfoEx->lpVerbW);
  }
  else
    if(HIWORD(commandInfo->lpVerb) == 0)
      commandOffset = LOWORD(commandInfo->lpVerb);
    else
      commandOffset = FindVerb(GetUnicodeString(commandInfo->lpVerb));

  if(commandOffset < 0 || commandOffset >= _commandMap.Size())
    return E_FAIL;

  const CCommandMapItem commandMapItem = _commandMap[commandOffset];
  ECommandInternalID commandInternalID = commandMapItem.CommandInternalID;

  try
  {
    switch(commandInternalID)
    {
      case kOpen:
      {
        UString params;
        params = GetProgramCommand();
        params += L" \"";
        params += _fileNames[0];
        params += L"\"";
        MyCreateProcess(params, 0, false, 0);
        break;
      }
      case kExtract:
      case kExtractHere:
      case kExtractTo:
      {
        ExtractArchives(_fileNames, commandMapItem.Folder,
            (commandInternalID == kExtract));
        break;
      }
      case kTest:
      {
        TestArchives(_fileNames);
        break;
      }
      case kCompress:
      case kCompressEmail:
      case kCompressTo7z:
      case kCompressTo7zEmail:
      case kCompressToZip:
      case kCompressToZipEmail:
      {
        bool email = 
            (commandInternalID == kCompressEmail) || 
            (commandInternalID == kCompressTo7zEmail) ||
            (commandInternalID == kCompressToZipEmail);
        bool showDialog = 
            (commandInternalID == kCompress) || 
            (commandInternalID == kCompressEmail);
        CompressFiles(commandMapItem.Folder, 
            commandMapItem.Archive, commandMapItem.ArchiveType,
            _fileNames, email, showDialog, false);
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

static void MyCopyString(void *dest, const wchar_t *src, bool writeInUnicode)
{
  if(writeInUnicode)
  {
    MyStringCopy((wchar_t *)dest, src);
  }
  else
    lstrcpyA((char *)dest, GetAnsiString(src));
}

STDMETHODIMP CZipContextMenu::GetCommandString(UINT_PTR commandOffset, UINT uType, 
    UINT * /* pwReserved */ , LPSTR pszName, UINT /* cchMax */)
{
  int cmdOffset = (int)commandOffset;
  switch(uType)
  { 
    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
      if(cmdOffset < 0 || cmdOffset >= _commandMap.Size())
        return S_FALSE;
      else 
        return S_OK;
  }
  if(cmdOffset < 0 || cmdOffset >= _commandMap.Size())
    return E_FAIL;
  if(uType == GCS_HELPTEXTA || uType == GCS_HELPTEXTW)
  {
    MyCopyString(pszName, _commandMap[cmdOffset].HelpString, uType == GCS_HELPTEXTW);
    return NO_ERROR;
  }
  if(uType == GCS_VERBA || uType == GCS_VERBW)
  {
    MyCopyString(pszName, _commandMap[cmdOffset].Verb, uType == GCS_VERBW);
    return NO_ERROR;
  }
  return E_FAIL;
}
