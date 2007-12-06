// PanelItemOpen.cpp

#include "StdAfx.h"

#include "resource.h" 

#include "Common/StringConvert.h" 
#include "Common/Random.h"
#include "Common/StringConvert.h"
#include "Common/AutoPtr.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"
#include "Windows/Synchronization.h"
#include "Windows/Error.h"
// #include "Windows/COM.h"

#include "ExtractCallback.h"
#include "IFolder.h"
#include "FileFolderPluginOpen.h"
#include "FormatUtils.h"
#include "Panel.h"
#include "RegistryUtils.h"

using namespace NWindows;
using namespace NSynchronization;
using namespace NFile;
using namespace NDirectory;

extern HWND g_HWND;
#ifndef _UNICODE
extern bool g_IsNT;
#endif

static wchar_t *kTempDirPrefix = L"7zO"; 

static const wchar_t *virusMessage = L"File looks like virus (file name has long spaces in name). 7-Zip will not open it";

static bool IsNameVirus(const UString &name)
{
  return (name.Find(L"     ") >= 0);
}

struct CTmpProcessInfo: public CTempFileInfo
{
  HANDLE ProcessHandle;
  HWND Window;
  UString FullPathFolderPrefix;
};

class CTmpProcessInfoRelease
{
  CTmpProcessInfo *_tmpProcessInfo;
public:
  bool _needDelete;
  CTmpProcessInfoRelease(CTmpProcessInfo &tmpProcessInfo):
      _tmpProcessInfo(&tmpProcessInfo), _needDelete(true) {}
  ~CTmpProcessInfoRelease()
  {
    if (_needDelete)
      _tmpProcessInfo->DeleteDirAndFile();
  }
};

HRESULT CPanel::OpenItemAsArchive(const UString &name, 
    const UString &folderPath, const UString &filePath, bool &encrypted)
{
  encrypted = false;
  CFolderLink folderLink;
  if (!NFile::NFind::FindFile(filePath, folderLink.FileInfo))
    return E_FAIL;
  if (folderLink.FileInfo.IsDirectory())
    return S_FALSE;

  folderLink.FilePath = filePath;
  folderLink.FolderPath = folderPath;

  CMyComPtr<IFolderFolder> newFolder;

  // _passwordIsDefined = false;
  // _password.Empty();

  NDLL::CLibrary library;
  RINOK(OpenFileFolderPlugin(filePath, &library, &newFolder, GetParent(), encrypted));
 
  folderLink.ParentFolder = _folder;
  folderLink.ItemName = name;
  _parentFolders.Add(folderLink);
  _parentFolders.Back().Library.Attach(_library.Detach());

  _folder.Release();
  _library.Free();
  _folder = newFolder;
  _library.Attach(library.Detach());

  return S_OK;
}

HRESULT CPanel::OpenItemAsArchive(const UString &name)
{
  bool encrypted;
  return OpenItemAsArchive(name, _currentFolderPrefix, _currentFolderPrefix + name, encrypted);
}

HRESULT CPanel::OpenItemAsArchive(int index)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  RINOK(OpenItemAsArchive(GetItemRelPath(index)));
  RefreshListCtrl();
  return S_OK;
}

HRESULT CPanel::OpenParentArchiveFolder()
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  if (_parentFolders.Size() < 2)
    return S_OK;
  CFolderLink &folderLink = _parentFolders.Back();
  NFind::CFileInfoW newFileInfo;
  if (NFind::FindFile(folderLink.FilePath, newFileInfo))
  {
    if (newFileInfo.Size != folderLink.FileInfo.Size || 
        CompareFileTime(&newFileInfo.LastWriteTime, 
        &folderLink.FileInfo.LastWriteTime) != 0)
    {
      UString message = MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE, 
          0x03020280, folderLink.ItemName);
      if (::MessageBoxW(HWND(*this), message, L"7-Zip", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
      {
        if (OnOpenItemChanged(folderLink.FolderPath, folderLink.ItemName) != S_OK)
        {
          ::MessageBoxW(HWND(*this), MyFormatNew(IDS_CANNOT_UPDATE_FILE, 
              0x03020281, folderLink.FilePath), L"7-Zip", MB_OK | MB_ICONSTOP);
          return S_OK;
        }
      }
    }
  }
  folderLink.DeleteDirAndFile();
  return S_OK;
}

static const wchar_t *kStartExtensions[] = 
{
  L"exe", L"bat", L"com",
  L"chm",
  L"msi", L"doc", L"xls", L"ppt", L"wps", L"wpt", L"wks", L"xlr", L"wdb",
  L"odt", L"ods",
  L"pdf"
};

static bool DoItemAlwaysStart(const UString &name)
{
  int extPos = name.ReverseFind('.');
  if (extPos < 0)
    return false;
  UString ext = name.Mid(extPos + 1);
  ext.MakeLower();
  for (int i = 0; i < sizeof(kStartExtensions) / sizeof(kStartExtensions[0]); i++)
    if (ext.Compare(kStartExtensions[i]) == 0)
      return true;
  return false;
}

static HANDLE StartEditApplication(const UString &path, HWND window)
{
  UString command;
  ReadRegEditor(command);
  if (command.IsEmpty())
  {
    if (!MyGetWindowsDirectory(command))
      return 0;
    NFile::NName::NormalizeDirPathPrefix(command);
    command += L"notepad.exe";
  }
  command = UString(L"\"") + command + UString(L"\"");
  command += L" \"";
  command += UString(path);
  command += L"\"";

  PROCESS_INFORMATION processInformation;
  BOOL result;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    STARTUPINFOA startupInfo;
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = 0;
    startupInfo.lpDesktop = 0;
    startupInfo.lpTitle = 0;
    startupInfo.dwFlags = 0;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = 0;
    
    result = ::CreateProcessA(NULL, (CHAR *)(const CHAR *)GetSystemString(command), 
      NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);
  }
  else
  #endif
  {
    STARTUPINFOW startupInfo;
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpReserved = 0;
    startupInfo.lpDesktop = 0;
    startupInfo.lpTitle = 0;
    startupInfo.dwFlags = 0;
    startupInfo.cbReserved2 = 0;
    startupInfo.lpReserved2 = 0;
    
    result = ::CreateProcessW(NULL, (WCHAR *)(const WCHAR *)command, 
      NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);
  }

  if (result != FALSE)
  {
    ::CloseHandle(processInformation.hThread);
    return processInformation.hProcess;
  }
  ::MessageBoxW(window, LangString(IDS_CANNOT_START_EDITOR, 0x03020282), 
      L"7-Zip", MB_OK  | MB_ICONSTOP);
  return 0;
}

#ifndef _UNICODE
typedef BOOL (WINAPI * ShellExecuteExWP)(LPSHELLEXECUTEINFOW lpExecInfo);
#endif

static HANDLE StartApplication(const UString &path, HWND window)
{
  UINT32 result;
  HANDLE hProcess;
  #ifndef _UNICODE
  if (g_IsNT)
  {
    SHELLEXECUTEINFOW execInfo;
    execInfo.cbSize = sizeof(execInfo);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    execInfo.hwnd = NULL;
    execInfo.lpVerb = NULL;
    execInfo.lpFile = path;
    execInfo.lpParameters = NULL;
    execInfo.lpDirectory = NULL;
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hProcess = 0;
    ShellExecuteExWP shellExecuteExW = (ShellExecuteExWP)
    ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "ShellExecuteExW");
    if (shellExecuteExW == 0)
      return 0;
    shellExecuteExW(&execInfo);
    result = (UINT32)(UINT_PTR)execInfo.hInstApp;
    hProcess = execInfo.hProcess;
  }
  else
  #endif
  {
    SHELLEXECUTEINFO execInfo;
    execInfo.cbSize = sizeof(execInfo);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
    execInfo.hwnd = NULL;
    execInfo.lpVerb = NULL;
    const CSysString sysPath = GetSystemString(path);
    execInfo.lpFile = sysPath;
    execInfo.lpParameters = NULL;
    execInfo.lpDirectory = NULL;
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hProcess = 0;
    ::ShellExecuteEx(&execInfo);
    result = (UINT32)(UINT_PTR)execInfo.hInstApp;
    hProcess = execInfo.hProcess;
  }
  if(result <= 32)
  {
    switch(result)
    {
      case SE_ERR_NOASSOC:
        ::MessageBoxW(window, 
          NError::MyFormatMessageW(::GetLastError()),
          // L"There is no application associated with the given file name extension",
          L"7-Zip", MB_OK | MB_ICONSTOP);
    }
  }
  return hProcess;
}

void CPanel::EditItem(int index)
{
  if (!_parentFolders.IsEmpty())
  {
    OpenItemInArchive(index, false, true, true);
    return;
  }
  HANDLE hProcess = StartEditApplication(_currentFolderPrefix + GetItemRelPath(index), (HWND)*this);
  if (hProcess != 0)
    ::CloseHandle(hProcess);
}

void CPanel::OpenFolderExternal(int index)
{
  HANDLE hProcess = StartApplication(GetFsPath() + GetItemRelPath(index), (HWND)*this);
  if (hProcess != 0)
    ::CloseHandle(hProcess);
}

void CPanel::OpenItem(int index, bool tryInternal, bool tryExternal)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  if (!_parentFolders.IsEmpty())
  {
    OpenItemInArchive(index, tryInternal, tryExternal, false);
    return;
  }
  UString name = GetItemRelPath(index);
  if (IsNameVirus(name))
  {
    MessageBoxMyError(virusMessage);
    return;
  }
  UString fullPath = _currentFolderPrefix + name;
  if (tryInternal)
    if (!tryExternal || !DoItemAlwaysStart(name))
      if (OpenItemAsArchive(index) == S_OK)
        return;
  if (tryExternal)
  {
    NDirectory::MySetCurrentDirectory(_currentFolderPrefix);
    HANDLE hProcess = StartApplication(fullPath, (HWND)*this);
    if (hProcess != 0)
      ::CloseHandle(hProcess);
  }
}
       
HRESULT CPanel::OnOpenItemChanged(const UString &folderPath, const UString &itemName)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return E_FAIL;
  }
  UStringVector fileNames;
  CRecordVector<const wchar_t *> fileNamePointers;
  fileNames.Add(itemName);
  fileNamePointers.Add(fileNames[0]);

  UString pathPrefix = folderPath;
  NName::NormalizeDirPathPrefix(pathPrefix);
  return folderOperations->CopyFrom(pathPrefix, &fileNamePointers.Front(),fileNamePointers.Size(), NULL);
}

LRESULT CPanel::OnOpenItemChanged(LPARAM lParam)
{
  CTmpProcessInfo &tmpProcessInfo = *(CTmpProcessInfo *)lParam;
  // LoadCurrentPath()
  if (tmpProcessInfo.FullPathFolderPrefix != _currentFolderPrefix)
    return 0;

  CSelectedState state;
  SaveSelectedState(state);

  HRESULT result = OnOpenItemChanged(tmpProcessInfo.FolderPath, tmpProcessInfo.ItemName);
  if (result != S_OK)
    return 0;
  RefreshListCtrl(state);
  return 1;
}

/*
class CTmpProcessInfoList
{
public:
  CObjectVector<CTmpProcessInfo> _items;
} g_TmpProcessInfoList;
*/

class CExitEventLauncher
{
public:
  NWindows::NSynchronization::CManualResetEvent _exitEvent;
  CExitEventLauncher()
  {
    if (_exitEvent.Create(false) != S_OK)
      throw 9387173;
  };
  ~CExitEventLauncher() {  _exitEvent.Set(); }
} g_ExitEventLauncher;

static THREAD_FUNC_DECL MyThreadFunction(void *param)
{
  CMyAutoPtr<CTmpProcessInfo> tmpProcessInfoPtr((CTmpProcessInfo *)param);
  CTmpProcessInfo *tmpProcessInfo = tmpProcessInfoPtr.get();

  HANDLE hProcess = tmpProcessInfo->ProcessHandle;
  HANDLE events[2] = { g_ExitEventLauncher._exitEvent, hProcess};
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  ::CloseHandle(hProcess);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return 0;
  if (waitResult != WAIT_OBJECT_0 + 1)
    return 1;
  Sleep(200);
  NFind::CFileInfoW newFileInfo;
  if (NFind::FindFile(tmpProcessInfo->FilePath, newFileInfo))
  {
    if (newFileInfo.Size != tmpProcessInfo->FileInfo.Size || 
        CompareFileTime(&newFileInfo.LastWriteTime, 
        &tmpProcessInfo->FileInfo.LastWriteTime) != 0)
    {
      UString message = MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE, 
          0x03020280, tmpProcessInfo->ItemName);
      if (::MessageBoxW(g_HWND, message, L"7-Zip", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
      {
        if (SendMessage(tmpProcessInfo->Window, kOpenItemChanged, 0, (LONG_PTR)tmpProcessInfo) != 1)
        {
          ::MessageBoxW(g_HWND, MyFormatNew(IDS_CANNOT_UPDATE_FILE, 
              0x03020281, tmpProcessInfo->FilePath), L"7-Zip", MB_OK | MB_ICONSTOP);
          return 0;
        }
      }
    }
  }
  tmpProcessInfo->DeleteDirAndFile();
  return 0;
}

void CPanel::OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
    bool editMode)
{
  const UString name = GetItemName(index);
  if (IsNameVirus(name))
  {
    MessageBoxMyError(virusMessage);
    return;
  }

  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  NFile::NDirectory::CTempDirectoryW tempDirectory;
  tempDirectory.Create(kTempDirPrefix);
  UString tempDir = tempDirectory.GetPath();
  UString tempDirNorm = tempDir;
  NFile::NName::NormalizeDirPathPrefix(tempDirNorm);

  CRecordVector<UInt32> indices;
  indices.Add(index);

  UStringVector messages;
  HRESULT result = CopyTo(indices, tempDirNorm, false, true, &messages);

  if (!messages.IsEmpty())
    return;
  if (result != S_OK)
  {
    if (result != E_ABORT)
      MessageBoxError(result);
    return;
  }

  UString tempFilePath = tempDirNorm + name;

  CMyAutoPtr<CTmpProcessInfo> tmpProcessInfoPtr(new CTmpProcessInfo());
  CTmpProcessInfo *tmpProcessInfo = tmpProcessInfoPtr.get();
  tmpProcessInfo->FolderPath = tempDir;
  tmpProcessInfo->FilePath = tempFilePath;
  if (!NFind::FindFile(tempFilePath, tmpProcessInfo->FileInfo))
    return;

  if (tryInternal)
  {
    if (!tryExternal || !DoItemAlwaysStart(name))
    {
      bool encrypted;
      if (OpenItemAsArchive(name, tempDir, tempFilePath, encrypted) == S_OK)
      {
        RefreshListCtrl();
        return;
      }
    }
  }

  CTmpProcessInfoRelease tmpProcessInfoRelease(*tmpProcessInfo);

  if (!tryExternal)
    return;

  HANDLE hProcess;
  if (editMode)
    hProcess = StartEditApplication(tempFilePath, (HWND)*this);
  else
    hProcess = StartApplication(tempFilePath, (HWND)*this);

  if (hProcess == 0)
    return;

  tmpProcessInfo->Window = (HWND)(*this);
  tmpProcessInfo->FullPathFolderPrefix = _currentFolderPrefix;
  tmpProcessInfo->ItemName = name;
  tmpProcessInfo->ProcessHandle = hProcess;

  NWindows::CThread thread;
  if (thread.Create(MyThreadFunction, tmpProcessInfo) != S_OK)
    throw 271824;
  tempDirectory.DisableDeleting();
  tmpProcessInfoPtr.release();
  tmpProcessInfoRelease._needDelete = false;
}

/*
static const UINT64 kTimeLimit = UINT64(10000000) * 3600 * 24;

static bool CheckDeleteItem(UINT64 currentFileTime, UINT64 folderFileTime)
{
  return (currentFileTime - folderFileTime > kTimeLimit &&
      folderFileTime - currentFileTime > kTimeLimit);
}

void DeleteOldTempFiles()
{
  UString tempPath;
  if(!NFile::NDirectory::MyGetTempPath(tempPath))
    throw 1;

  SYSTEMTIME systemTime;
  ::GetSystemTime(&systemTime);
  UINT64 currentFileTime;
  if(!::SystemTimeToFileTime(&systemTime, (FILETIME *)&currentFileTime))
    throw 2;
  UString searchWildCard = tempPath + kTempDirPrefix + L"*.tmp";
  searchWildCard += WCHAR(NName::kAnyStringWildcard);
  NFind::CEnumeratorW enumerator(searchWildCard);
  NFind::CFileInfoW fileInfo;
  while(enumerator.Next(fileInfo))
  {
    if (!fileInfo.IsDirectory())
      continue;
    const UINT64 &creationTime = *(const UINT64 *)(&fileInfo.CreationTime);
    if(CheckDeleteItem(creationTime, currentFileTime))
      RemoveDirectoryWithSubItems(tempPath + fileInfo.Name);
  }
}
*/
