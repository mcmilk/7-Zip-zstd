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
#include "Windows/Error.h"
#include "Windows/COM.h"

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

static inline UINT GetCurrentFileCodePage()
  {  return AreFileApisANSI() ? CP_ACP : CP_OEMCP;}

static LPCTSTR kTempDirPrefix = TEXT("7zO"); 

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

  CMyComPtr<IFolderFolder> newFolder;

  // _passwordIsDefined = false;
  // _password.Empty();

  NDLL::CLibrary library;
  RINOK(OpenFileFolderPlugin(GetUnicodeString(filePath), 
      &library, &newFolder, GetParent()));
 
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
  return OpenItemAsArchive(name, GetSystemString(_currentFolderPrefix), 
      GetSystemString(_currentFolderPrefix + name));
}

HRESULT CPanel::OpenItemAsArchive(int index)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  RINOK(OpenItemAsArchive(GetItemName(index)));
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
    if (!MyGetWindowsDirectory(command))
      return 0;
    NFile::NName::NormalizeDirPathPrefix(command);
    command += TEXT("notepad.exe");
  }
  command = CSysString(TEXT("\"")) + command + CSysString(TEXT("\""));
  command += TEXT(" \"");
  command += path;
  command += TEXT("\"");

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
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT;
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
  HANDLE hProcess = StartApplication(fullPath, (HWND)*this);
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
  CSysString fullPath = GetSystemString((_currentFolderPrefix + 
      GetItemName(index)), GetCurrentFileCodePage());
  if (tryInternal)
    if (!tryExternal || !DoItemAlwaysStart(GetItemName(index)))
      if (OpenItemAsArchive(index) == S_OK)
        return;
  if (tryExternal)
  {
    ::SetCurrentDirectory(GetSystemString(_currentFolderPrefix));
    HANDLE hProcess = StartApplication(fullPath, (HWND)*this);
    if (hProcess != 0)
      ::CloseHandle(hProcess);
  }
}
       
HRESULT CPanel::OnOpenItemChanged(const CSysString &folderPath, const UString &itemName)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
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

struct CThreadExtractInArchive
{
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UINT32> Indices;
  UString DestPath;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderOperationsExtractCallback> ExtractCallback;
  HRESULT Result;
  
  DWORD Extract()
  {
    NCOM::CComInitializer comInitializer;
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    Result = FolderOperations->CopyTo(
        &Indices.Front(), Indices.Size(), 
        DestPath, ExtractCallback);
    // ExtractCallbackSpec->DestroyWindows();
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }
  
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtractInArchive *)param)->Extract();
  }
};

void CPanel::OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
    bool editMode)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  CSysString tempDir;
  if (!CreateTempDirectory(kTempDirPrefix, tempDir))
    return;

  CThreadExtractInArchive extracter;

  extracter.ExtractCallbackSpec = new CExtractCallbackImp;
  extracter.ExtractCallback = extracter.ExtractCallbackSpec;
  extracter.ExtractCallbackSpec->ParentWindow = GetParent();
  
  // extracter.ExtractCallbackSpec->_appTitle.Window = extracter.ExtractCallbackSpec->_parentWindow;
  // extracter.ExtractCallbackSpec->_appTitle.Title = progressWindowTitle;
  // extracter.ExtractCallbackSpec->_appTitle.AddTitle = title + CSysString(TEXT(" "));

  extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
  extracter.ExtractCallbackSpec->Init();
  extracter.Indices.Add(index);
  extracter.DestPath = GetUnicodeString(tempDir + NFile::NName::kDirDelimiter);
  extracter.FolderOperations = folderOperations;

  CThread extractThread;
  if (!extractThread.Create(CThreadExtractInArchive::MyThreadFunction, &extracter))
    throw 271824;
  extracter.ExtractCallbackSpec->StartProgressDialog(LangLoadStringW(IDS_OPENNING, 0x03020283));

  if (extracter.Result != S_OK || extracter.ExtractCallbackSpec->Messages.Size() != 0)
  {
    if (extracter.Result != S_OK)
      if (extracter.Result != E_ABORT)
      {
        // MessageBox(L"Can not extract item");
        // NError::MyFormatMessage(systemError.ErrorCode, message);
        MessageBoxError(extracter.Result, L"7-Zip");
      }
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
