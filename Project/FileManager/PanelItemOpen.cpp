// PanelItemOpen.cpp

#include "StdAfx.h"

#include "resource.h" 

#include "Common/StringConvert.h" 
#include "Common/Random.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Thread.h"
#include "Windows/Synchronization.h"
#include "Windows/System.h"
#include "Windows/Error.h"

#include "../Archiver/Common/IArchiveHandler2.h" 

#include "ExtractCallback.h"
#include "FolderInterface.h"
#include "FileFolderPluginOpen.h"
#include "FormatUtils.h"
#include "Panel.h"
#include "RegistryUtils.h"

using namespace NWindows;
using namespace NSynchronization;
using namespace NFile;
using namespace NDirectory;

extern HWND g_HWND;

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;}

static LPCTSTR kTempDirPrefix = _T("7zO"); 

struct CTmpProcessInfo: public CTempFileInfo
{
  HANDLE ProcessHandle;
  HWND Window;
  UString FullPathFolderPrefix;
};

class CTmpProcessInfoRelease
{
  CTmpProcessInfo &_tmpProcessInfo;
public:
  bool _needDelete;
  CTmpProcessInfoRelease(CTmpProcessInfo &tmpProcessInfo):
      _tmpProcessInfo(tmpProcessInfo), _needDelete(true) {}
  ~CTmpProcessInfoRelease()
  {
    if (_needDelete)
      _tmpProcessInfo.DeleteDirAndFile();
  }
};

HRESULT CPanel::OpenItemAsArchive(const UString &name, 
    const CSysString &folderPath,
    const CSysString &filePath)
{
  CFolderLink folderLink;
  if (!NFile::NFind::FindFile(filePath, folderLink.FileInfo))
    return E_FAIL;
  if (folderLink.FileInfo.IsDirectory())
    return S_FALSE;

  folderLink.FilePath = filePath;
  folderLink.FolderPath = folderPath;

  CComPtr<IFolderFolder> newFolder;
  RETURN_IF_NOT_S_OK(OpenFileFolderPlugin(GetUnicodeString(filePath), &newFolder));
 
  folderLink.ParentFolder = _folder;
  folderLink.ItemName = name;

  _parentFolders.Add(folderLink);
  _folder.Release();
  _folder = newFolder;

  return S_OK;
}

HRESULT CPanel::OpenItemAsArchive(const UString &name)
{
  return OpenItemAsArchive(name, GetSystemString(_currentFolderPrefix), 
      GetSystemString(_currentFolderPrefix + name));
}

HRESULT CPanel::OpenItemAsArchive(int index)
{
  RETURN_IF_NOT_S_OK(OpenItemAsArchive(GetItemName(index)));
  RefreshListCtrl();
  return S_OK;
}

HRESULT CPanel::OpenParentArchiveFolder()
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  if (_parentFolders.Size() < 2)
    return S_OK;
  CFolderLink &folderLink = _parentFolders.Back();
  NFind::CFileInfo newFileInfo;
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
              0x03020281, GetUnicodeString(folderLink.FilePath)), L"7-Zip", MB_OK | MB_ICONSTOP);
          return S_OK;
        }
      }
    }
  }
  folderLink.DeleteDirAndFile();
  return S_OK;
}

static bool DoItemAlwaysStart(const UString &name)
{
  int extPos = name.ReverseFind('.');
  if (extPos < 0)
    return false;
  const UString ext = name.Mid(extPos + 1);
  return  (ext == UString(L"exe") || ext == UString(L"bat") || ext == UString(L"com"));
}

static HANDLE StartEditApplication(CSysString &path, HWND window)
{
  CSysString command;
  ReadRegEditor(command);
  if (command.IsEmpty())
  {
    if (!NSystem::MyGetWindowsDirectory(command))
      return 0;
    NFile::NName::NormalizeDirPathPrefix(command);
    command += _T("notepad.exe");
  }
  command = CSysString(_T("\"")) + command + CSysString(_T("\""));
  command += _T(" \"");
  command += path;
  command += _T("\"");

  STARTUPINFO startupInfo;
  startupInfo.cb = sizeof(startupInfo);
  startupInfo.lpReserved = 0;
  startupInfo.lpDesktop = 0;
  startupInfo.lpTitle = 0;
  startupInfo.dwFlags = 0;
  startupInfo.cbReserved2 = 0;
  startupInfo.lpReserved2 = 0;
  
  PROCESS_INFORMATION processInformation;
  BOOL result = ::CreateProcess(NULL, (TCHAR *)(const TCHAR *)command, 
      NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);
  if (result != FALSE)
  {
    ::CloseHandle(processInformation.hThread);
    return processInformation.hProcess;
  }
  ::MessageBoxW(window, LangLoadStringW(IDS_CANNOT_START_EDITOR, 0x03020282), 
      L"7-Zip", MB_OK  | MB_ICONSTOP);
  return 0;
}

static HANDLE StartApplication(CSysString &path, HWND window)
{
  SHELLEXECUTEINFO execInfo;
  execInfo.cbSize = sizeof(execInfo);
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // 
  execInfo.hwnd = NULL;
  execInfo.lpVerb = NULL;
  execInfo.lpFile = path;
  execInfo.lpParameters = NULL;
  execInfo.lpDirectory = NULL;
  execInfo.nShow = SW_SHOWNORMAL;
  execInfo.hProcess = 0;
  bool success = BOOLToBool(::ShellExecuteEx(&execInfo));
  UINT32 result = (UINT32)execInfo.hInstApp;
  if(result <= 32)
  {
    switch(result)
    {
      case SE_ERR_NOASSOC:
        ::MessageBox(window, 
          NError::MyFormatMessage(::GetLastError()),
          // TEXT("There is no application associated with the given file name extension"),
          TEXT("7-Zip"), MB_OK | MB_ICONSTOP);
    }
  }
  return execInfo.hProcess;
}

void CPanel::EditItem(int index)
{
  if (!_parentFolders.IsEmpty())
  {
    OpenItemInArchive(index, false, true, true);
    return;
  }
  CSysString fullPath = GetSystemString((_currentFolderPrefix + 
      GetItemName(index)), GetCurrentFileCodePage());
  HANDLE hProcess = StartEditApplication(fullPath, (HWND)*this);
  if (hProcess != 0)
    ::CloseHandle(hProcess);
}

void CPanel::OpenFolderExternal(int index)
{
  CSysString fullPath = GetSystemString((_currentFolderPrefix + 
      GetItemName(index)), GetCurrentFileCodePage());
  StartApplication(fullPath, (HWND)*this);
}

void CPanel::OpenItem(int index, bool tryInternal, bool tryExternal)
{
  if (!_parentFolders.IsEmpty())
  {
    OpenItemInArchive(index, tryInternal, tryExternal, false);
    return;
  }
  CSysString fullPath = GetSystemString((_currentFolderPrefix + 
      GetItemName(index)), GetCurrentFileCodePage());
  if (tryInternal)
    if (!tryExternal || !DoItemAlwaysStart(GetItemName(index)))
      if (OpenItemAsArchive(index) == S_OK)
        return;
  if (tryExternal)
  {
    HANDLE hProcess = StartApplication(fullPath, (HWND)*this);
    if (hProcess != 0)
      ::CloseHandle(hProcess);
  }
}
       
HRESULT CPanel::OnOpenItemChanged(const CSysString &folderPath, const UString &itemName)
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return E_FAIL;
  }
  UStringVector fileNames;
  CRecordVector<const wchar_t *> fileNamePointers;
  fileNames.Add(itemName);
  fileNamePointers.Add(fileNames[0]);

  // SetCurrentDirectory(tmpProcessInfo.FolderPath);
  CSysString pathPrefix = folderPath;
  NName::NormalizeDirPathPrefix(pathPrefix);
  return folderOperations->CopyFrom(
      GetUnicodeString(pathPrefix),
      &fileNamePointers.Front(),
      fileNamePointers.Size(),
      NULL);
}

LRESULT CPanel::OnOpenItemChanged(LPARAM lParam)
{
  CTmpProcessInfo &tmpProcessInfo = *(CTmpProcessInfo *)lParam;
  // LoadCurrentPath()
  if (tmpProcessInfo.FullPathFolderPrefix != _currentFolderPrefix)
    return 0;
  HRESULT result = OnOpenItemChanged(tmpProcessInfo.FolderPath, tmpProcessInfo.ItemName);
  if (result != S_OK)
    return 0;
  RefreshListCtrlSaveFocused();
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
  CManualResetEvent _exitEvent;
  CExitEventLauncher(): _exitEvent(false) {};
  ~CExitEventLauncher() {  _exitEvent.Set(); }
} g_ExitEventLauncher;

static DWORD WINAPI MyThreadFunction(void *param)
{
  // CTmpProcessInfo *tmpProcessInfo = (CTmpProcessInfo *)param;
  std::auto_ptr<CTmpProcessInfo> tmpProcessInfo((CTmpProcessInfo *)param);
  HANDLE hProcess = tmpProcessInfo->ProcessHandle;
  HANDLE events[2] = { g_ExitEventLauncher._exitEvent, hProcess};
  DWORD waitResult = ::WaitForMultipleObjects(2, events, FALSE, INFINITE);
  ::CloseHandle(hProcess);
  if (waitResult == WAIT_OBJECT_0 + 0)
    return 0;
  if (waitResult != WAIT_OBJECT_0 + 1)
    return 1;
  Sleep(200);
  NFind::CFileInfo newFileInfo;
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
        if (SendMessage(tmpProcessInfo->Window, kOpenItemChanged, 0, (LONG_PTR)tmpProcessInfo.get()) != 1)
        {
          ::MessageBoxW(g_HWND, MyFormatNew(IDS_CANNOT_UPDATE_FILE, 
              0x03020281, GetUnicodeString(tmpProcessInfo->FilePath)), L"7-Zip", MB_OK | MB_ICONSTOP);
          return 0;
        }
      }
    }
  }
  tmpProcessInfo->DeleteDirAndFile();
  return 0;
}

static CCriticalSection g_CriticalSection;

void CPanel::OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
    bool editMode)
{
  CComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(&folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  CSysString tempDir;
  if (!CreateTempDirectory(kTempDirPrefix, tempDir))
    return;

  CComObjectNoLock<CExtractCallbackImp> *extractCallbackSpec =
      new CComObjectNoLock<CExtractCallbackImp>;
  CComPtr<IFolderOperationsExtractCallback> extractCallback(extractCallbackSpec);
  extractCallbackSpec->_parentWindow = GetParent();
  extractCallbackSpec->StartProgressDialog(LangLoadString(IDS_OPENNING, 0x03020283));
  extractCallbackSpec->Init(NExtractionMode::NOverwrite::kWithoutPrompt, false, L"");

  CRecordVector<UINT32> indices;
  indices.Add(index);
  if (folderOperations->CopyTo(&indices.Front(), indices.Size(),
      GetUnicodeString(tempDir + NFile::NName::kDirDelimiter), 
      extractCallback) != S_OK)
  {
    // MessageBox(TEXT("Can not extract item"));
    return;
  }

  UString name = GetItemName(index);
  CSysString tempFileName = tempDir + NFile::NName::kDirDelimiter + 
      GetSystemString(name);

  std::auto_ptr<CTmpProcessInfo> tmpProcessInfo(new CTmpProcessInfo());
  tmpProcessInfo->FolderPath = tempDir;
  tmpProcessInfo->FilePath = tempFileName;
  if (!NFind::FindFile(tempFileName, tmpProcessInfo->FileInfo))
    return;

  if (tryInternal)
  {
    if (!tryExternal || !DoItemAlwaysStart(name))
      if (OpenItemAsArchive(name, tempDir, tempFileName) == S_OK)
      {
        RefreshListCtrl();
        return;
      }
  }

  CTmpProcessInfoRelease tmpProcessInfoRelease(*tmpProcessInfo);

  if (!tryExternal)
    return;

  HANDLE hProcess;
  if (editMode)
    hProcess = StartEditApplication(tempFileName, (HWND)*this);
  else
    hProcess = StartApplication(tempFileName, (HWND)*this);

  if (hProcess == 0)
    return;

  tmpProcessInfo->Window = (HWND)(*this);
  tmpProcessInfo->FullPathFolderPrefix = _currentFolderPrefix;
  tmpProcessInfo->ItemName = name;
  tmpProcessInfo->ProcessHandle = hProcess;
  
  CThread thread;
  if (!thread.Create(MyThreadFunction, tmpProcessInfo.get()))
    throw 271824;
  tmpProcessInfo.release();
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
  CSysString tempPath;
  if(!NFile::NDirectory::MyGetTempPath(tempPath))
    throw 1;

  SYSTEMTIME systemTime;
  ::GetSystemTime(&systemTime);
  UINT64 currentFileTime;
  if(!::SystemTimeToFileTime(&systemTime, (FILETIME *)&currentFileTime))
    throw 2;
  CSysString searchWildCard = tempPath + kTempDirPrefix + TEXT("*.tmp");
  searchWildCard += TCHAR(NName::kAnyStringWildcard);
  NFind::CEnumerator enumerator(searchWildCard);
  NFind::CFileInfo fileInfo;
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
