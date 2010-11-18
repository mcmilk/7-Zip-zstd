// PanelItemOpen.cpp

#include "StdAfx.h"

#include "Common/AutoPtr.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Process.h"
#include "Windows/PropVariant.h"
#include "Windows/Thread.h"

#include "../Common/ExtractingFilePath.h"

#include "App.h"

#include "FileFolderPluginOpen.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "RegistryUtils.h"
#include "UpdateCallback100.h"

#include "resource.h"

using namespace NWindows;
using namespace NSynchronization;
using namespace NFile;
using namespace NDirectory;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

static wchar_t *kTempDirPrefix = L"7zO";


static bool IsNameVirus(const UString &name)
{
  return (name.Find(L"     ") >= 0);
}

struct CTmpProcessInfo: public CTempFileInfo
{
  HANDLE ProcessHandle;
  HWND Window;
  UString FullPathFolderPrefix;
  bool UsePassword;
  UString Password;
  CTmpProcessInfo(): UsePassword(false) {}
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

HRESULT CPanel::OpenItemAsArchive(IInStream *inStream,
    const CTempFileInfo &tempFileInfo,
    const UString &virtualFilePath,
    const UString &arcFormat,
    bool &encrypted)
{
  encrypted = false;
  CFolderLink folderLink;
  (CTempFileInfo &)folderLink = tempFileInfo;
  if (inStream)
    folderLink.IsVirtual = true;
  else
  {
    if (!folderLink.FileInfo.Find(folderLink.FilePath))
      return ::GetLastError();
    if (folderLink.FileInfo.IsDir())
      return S_FALSE;
    folderLink.IsVirtual = false;
  }

  folderLink.VirtualPath = virtualFilePath;

  CMyComPtr<IFolderFolder> newFolder;

  // _passwordIsDefined = false;
  // _password.Empty();

  NDLL::CLibrary library;

  UString password;
  RINOK(OpenFileFolderPlugin(inStream,
      folderLink.FilePath.IsEmpty() ? virtualFilePath : folderLink.FilePath,
      arcFormat,
      &library, &newFolder, GetParent(), encrypted, password));
 
  folderLink.Password = password;
  folderLink.UsePassword = encrypted;

  folderLink.ParentFolder = _folder;
  _parentFolders.Add(folderLink);
  _parentFolders.Back().Library.Attach(_library.Detach());

  _folder.Release();
  _library.Free();
  _folder = newFolder;
  _library.Attach(library.Detach());

  _flatMode = _flatModeForArc;

  CMyComPtr<IGetFolderArcProps> getFolderArcProps;
  _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
  if (getFolderArcProps)
  {
    CMyComPtr<IFolderArcProps> arcProps;
    getFolderArcProps->GetFolderArcProps(&arcProps);
    if (arcProps)
    {
      UString s;
      UInt32 numLevels;
      if (arcProps->GetArcNumLevels(&numLevels) != S_OK)
        numLevels = 0;
      for (UInt32 level2 = 0; level2 < numLevels; level2++)
      {
        UInt32 level = numLevels - 1 - level2;
        PROPID propIDs[] = { kpidError, kpidPath, kpidType } ;
        UString values[3];
        for (Int32 i = 0; i < 3; i++)
        {
          CMyComBSTR name;
          NCOM::CPropVariant prop;
          if (arcProps->GetArcProp(level, propIDs[i], &prop) != S_OK)
            continue;
          if (prop.vt != VT_EMPTY)
            values[i] = (prop.vt == VT_BSTR) ? prop.bstrVal : L"?";
        }
        if (!values[0].IsEmpty())
        {
          if (!s.IsEmpty())
            s += L"--------------------\n";
          s += values[0]; s += L"\n\n[";
          s += values[2]; s += L"] ";
          s += values[1]; s += L"\n";
        }
      }
      if (!s.IsEmpty())
        MessageBox(s);
    }
  }

  return S_OK;
}

HRESULT CPanel::OpenItemAsArchive(const UString &name, const UString &arcFormat, bool &encrypted)
{
  CTempFileInfo tfi;
  tfi.ItemName = name;
  tfi.FolderPath = _currentFolderPrefix;
  tfi.FilePath = _currentFolderPrefix + name;
  return OpenItemAsArchive(NULL, tfi, _currentFolderPrefix + name, arcFormat, encrypted);
}

HRESULT CPanel::OpenItemAsArchive(int index)
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  bool encrypted;
  RINOK(OpenItemAsArchive(GetItemRelPath(index), UString(), encrypted));
  RefreshListCtrl();
  return S_OK;
}

HRESULT CPanel::OpenParentArchiveFolder()
{
  CDisableTimerProcessing disableTimerProcessing1(*this);
  if (_parentFolders.Size() < 2)
    return S_OK;
  const CFolderLink &folderLink = _parentFolders.Back();
  NFind::CFileInfoW newFileInfo;
  if (newFileInfo.Find(folderLink.FilePath))
  {
    if (folderLink.WasChanged(newFileInfo))
    {
      UString message = MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE,
          0x03020280, folderLink.ItemName);
      if (::MessageBoxW(HWND(*this), message, L"7-Zip", MB_OKCANCEL | MB_ICONQUESTION) == IDOK)
      {
        if (OnOpenItemChanged(folderLink.FolderPath, folderLink.ItemName,
            folderLink.UsePassword, folderLink.Password) != S_OK)
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

static const char *kStartExtensions =
  #ifdef UNDER_CE
  " cab"
  #endif
  " exe bat com"
  " chm"
  " msi doc xls ppt pps wps wpt wks xlr wdb vsd pub"

  " docx docm dotx dotm xlsx xlsm xltx xltm xlsb xps"
  " xlam pptx pptm potx potm ppam ppsx ppsm xsn"
  " mpp"
  " msg"
  " dwf"

  " flv swf"
  
  " odt ods"
  " wb3"
  " pdf"
  " ";

static bool FindExt(const char *p, const UString &name)
{
  int extPos = name.ReverseFind('.');
  if (extPos < 0)
    return false;
  UString ext = name.Mid(extPos + 1);
  ext.MakeLower();
  AString ext2 = UnicodeStringToMultiByte(ext);
  for (int i = 0; p[i] != 0;)
  {
    int j;
    for (j = i; p[j] != ' '; j++);
    if (ext2.Length() == j - i && memcmp(p + i, (const char *)ext2, ext2.Length()) == 0)
      return true;
    i = j + 1;
  }
  return false;
}

static bool DoItemAlwaysStart(const UString &name)
{
  return FindExt(kStartExtensions, name);
}

static UString GetQuotedString(const UString &s)
{
  return UString(L'\"') + s + UString(L'\"');
}

static HRESULT StartEditApplication(const UString &path, HWND window, CProcess &process)
{
  UString command;
  ReadRegEditor(command);
  if (command.IsEmpty())
  {
    #ifdef UNDER_CE
    command = L"\\Windows\\";
    #else
    if (!MyGetWindowsDirectory(command))
      return 0;
    NFile::NName::NormalizeDirPathPrefix(command);
    #endif
    command += L"notepad.exe";
  }

  HRESULT res = process.Create(command, GetQuotedString(path), NULL);
  if (res != SZ_OK)
    ::MessageBoxW(window, LangString(IDS_CANNOT_START_EDITOR, 0x03020282), L"7-Zip", MB_OK  | MB_ICONSTOP);
  return res;
}

void CApp::DiffFiles()
{
  const CPanel &panel = GetFocusedPanel();
  
  CRecordVector<UInt32> indices;
  panel.GetSelectedItemsIndices(indices);

  UString path1, path2;
  if (indices.Size() == 2)
  {
    path1 = panel.GetItemFullPath(indices[0]);
    path2 = panel.GetItemFullPath(indices[1]);
  }
  else if (indices.Size() == 1 && NumPanels >= 2)
  {
    const CPanel &destPanel = Panels[1 - LastFocusedPanel];
    path1 = panel.GetItemFullPath(indices[0]);
    const UString relPath = panel.GetItemRelPath(indices[0]);
    CRecordVector<UInt32> indices2;
    destPanel.GetSelectedItemsIndices(indices2);
    if (indices2.Size() == 1)
      path2 = destPanel.GetItemFullPath(indices2[0]);
    else
      path2 = destPanel._currentFolderPrefix + relPath;
  }
  else
    return;

  UString command;
  ReadRegDiff(command);
  if (command.IsEmpty())
    return;

  UString param = GetQuotedString(path1) + L' ' + GetQuotedString(path2);

  HRESULT res = MyCreateProcess(command, param);
  if (res == SZ_OK)
    return;
  ::MessageBoxW(_window, LangString(IDS_CANNOT_START_EDITOR, 0x03020282), L"7-Zip", MB_OK  | MB_ICONSTOP);
}

#ifndef _UNICODE
typedef BOOL (WINAPI * ShellExecuteExWP)(LPSHELLEXECUTEINFOW lpExecInfo);
#endif

static HRESULT StartApplication(const UString &dir, const UString &path, HWND window, CProcess &process)
{
  UINT32 result;
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
    execInfo.lpDirectory = dir.IsEmpty() ? NULL : (LPCWSTR)dir;
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hProcess = 0;
    ShellExecuteExWP shellExecuteExW = (ShellExecuteExWP)
    ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "ShellExecuteExW");
    if (shellExecuteExW == 0)
      return 0;
    shellExecuteExW(&execInfo);
    result = (UINT32)(UINT_PTR)execInfo.hInstApp;
    process.Attach(execInfo.hProcess);
  }
  else
  #endif
  {
    SHELLEXECUTEINFO execInfo;
    execInfo.cbSize = sizeof(execInfo);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS
      #ifndef UNDER_CE
      | SEE_MASK_FLAG_DDEWAIT
      #endif
      ;
    execInfo.hwnd = NULL;
    execInfo.lpVerb = NULL;
    const CSysString sysPath = GetSystemString(path);
    const CSysString sysDir = GetSystemString(dir);
    execInfo.lpFile = sysPath;
    execInfo.lpParameters = NULL;
    execInfo.lpDirectory =
    #ifdef UNDER_CE
    NULL
    #else
    sysDir.IsEmpty() ? NULL : (LPCTSTR)sysDir
    #endif
    ;
    execInfo.nShow = SW_SHOWNORMAL;
    execInfo.hProcess = 0;
    ::ShellExecuteEx(&execInfo);
    result = (UINT32)(UINT_PTR)execInfo.hInstApp;
    process.Attach(execInfo.hProcess);
  }
  if (result <= 32)
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
  return S_OK;
}

static void StartApplicationDontWait(const UString &dir, const UString &path, HWND window)
{
  CProcess process;
  StartApplication(dir, path, window, process);
}

void CPanel::EditItem(int index)
{
  if (!_parentFolders.IsEmpty())
  {
    OpenItemInArchive(index, false, true, true);
    return;
  }
  CProcess process;
  StartEditApplication(GetItemFullPath(index), (HWND)*this, process);
}

void CPanel::OpenFolderExternal(int index)
{
  UString fsPrefix = GetFsPath();
  UString name;
  if (index == kParentIndex)
  {
    int pos = fsPrefix.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (pos >= 0 && pos == fsPrefix.Length() - 1)
    {
      UString s = fsPrefix.Left(pos);
      pos = s.ReverseFind(WCHAR_PATH_SEPARATOR);
      if (pos >= 0)
        fsPrefix = s.Left(pos + 1);
    }
    name = fsPrefix;
  }
  else
    name = fsPrefix + GetItemRelPath(index) + WCHAR_PATH_SEPARATOR;
  StartApplicationDontWait(fsPrefix, name, (HWND)*this);
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
    MessageBoxErrorLang(IDS_VIRUS, 0x03020284);
    return;
  }
  UString prefix = GetFsPath();
  UString fullPath = prefix + name;
  if (tryInternal)
    if (!tryExternal || !DoItemAlwaysStart(name))
    {
      HRESULT res = OpenItemAsArchive(index);
      if (res == S_OK || res == E_ABORT)
        return;
      if (res != S_FALSE)
      {
        MessageBoxError(res);
        return;
      }
    }
  if (tryExternal)
  {
    // SetCurrentDirectory opens HANDLE to folder!!!
    // NDirectory::MySetCurrentDirectory(prefix);
    StartApplicationDontWait(prefix, fullPath, (HWND)*this);
  }
}

class CThreadCopyFrom: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  UString PathPrefix;
  UString Name;

  CMyComPtr<IFolderOperations> FolderOperations;
  CMyComPtr<IProgress> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
};
  
HRESULT CThreadCopyFrom::ProcessVirt()
{
  UStringVector fileNames;
  CRecordVector<const wchar_t *> fileNamePointers;
  fileNames.Add(Name);
  fileNamePointers.Add(fileNames[0]);
  return FolderOperations->CopyFrom(PathPrefix, &fileNamePointers.Front(), fileNamePointers.Size(), UpdateCallback);
}
      
HRESULT CPanel::OnOpenItemChanged(const UString &folderPath, const UString &itemName,
    bool usePassword, const UString &password)
{
  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return E_FAIL;
  }

  CThreadCopyFrom t;
  t.UpdateCallbackSpec = new CUpdateCallback100Imp;
  t.UpdateCallback = t.UpdateCallbackSpec;
  t.UpdateCallbackSpec->ProgressDialog = &t.ProgressDialog;
  t.Name = itemName;
  t.PathPrefix = folderPath;
  NName::NormalizeDirPathPrefix(t.PathPrefix);
  t.FolderOperations = folderOperations;
  t.UpdateCallbackSpec->Init(usePassword, password);
  RINOK(t.Create(itemName, (HWND)*this));
  return t.Result;
}

LRESULT CPanel::OnOpenItemChanged(LPARAM lParam)
{
  CTmpProcessInfo &tmpProcessInfo = *(CTmpProcessInfo *)lParam;
  // LoadCurrentPath()
  if (tmpProcessInfo.FullPathFolderPrefix != _currentFolderPrefix)
    return 0;

  CSelectedState state;
  SaveSelectedState(state);

  HRESULT result = OnOpenItemChanged(tmpProcessInfo.FolderPath, tmpProcessInfo.ItemName,
      tmpProcessInfo.UsePassword, tmpProcessInfo.Password);
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
  const CTmpProcessInfo *tmpProcessInfo = tmpProcessInfoPtr.get();

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
  if (newFileInfo.Find(tmpProcessInfo->FilePath))
  {
    if (tmpProcessInfo->WasChanged(newFileInfo))
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

void CPanel::OpenItemInArchive(int index, bool tryInternal, bool tryExternal, bool editMode)
{
  const UString name = GetItemName(index);
  if (IsNameVirus(name))
  {
    MessageBoxErrorLang(IDS_VIRUS, 0x03020284);
    return;
  }

  CMyComPtr<IFolderOperations> folderOperations;
  if (_folder.QueryInterface(IID_IFolderOperations, &folderOperations) != S_OK)
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }

  bool tryAsArchive = tryInternal && (!tryExternal || !DoItemAlwaysStart(name));

  UString fullVirtPath = _currentFolderPrefix + name;

  NFile::NDirectory::CTempDirectoryW tempDirectory;
  tempDirectory.Create(kTempDirPrefix);
  UString tempDir = tempDirectory.GetPath();
  UString tempDirNorm = tempDir;
  NFile::NName::NormalizeDirPathPrefix(tempDirNorm);

  UString tempFilePath = tempDirNorm + GetCorrectFsPath(name);

  CTempFileInfo tempFileInfo;
  tempFileInfo.ItemName = name;
  tempFileInfo.FolderPath = tempDir;
  tempFileInfo.FilePath = tempFilePath;
  tempFileInfo.NeedDelete = true;

  if (tryAsArchive)
  {
    CMyComPtr<IInArchiveGetStream> getStream;
    _folder.QueryInterface(IID_IInArchiveGetStream, &getStream);
    if (getStream)
    {
      CMyComPtr<ISequentialInStream> subSeqStream;
      getStream->GetStream(index, &subSeqStream);
      if (subSeqStream)
      {
        CMyComPtr<IInStream> subStream;
        subSeqStream.QueryInterface(IID_IInStream, &subStream);
        if (subStream)
        {
          bool encrypted;
          if (OpenItemAsArchive(subStream, tempFileInfo, fullVirtPath, UString(), encrypted) == S_OK)
          {
            tempDirectory.DisableDeleting();
            RefreshListCtrl();
            return;
          }
        }
      }
    }
  }


  CRecordVector<UInt32> indices;
  indices.Add(index);

  UStringVector messages;

  bool usePassword = false;
  UString password;
  if (_parentFolders.Size() > 0)
  {
    const CFolderLink &fl = _parentFolders.Back();
    usePassword = fl.UsePassword;
    password = fl.Password;
  }

  HRESULT result = CopyTo(indices, tempDirNorm, false, true, &messages, usePassword, password);

  if (_parentFolders.Size() > 0)
  {
    CFolderLink &fl = _parentFolders.Back();
    fl.UsePassword = usePassword;
    fl.Password = password;
  }

  if (!messages.IsEmpty())
    return;
  if (result != S_OK)
  {
    if (result != E_ABORT)
      MessageBoxError(result);
    return;
  }


  if (tryAsArchive)
  {
    bool encrypted;
    if (OpenItemAsArchive(NULL, tempFileInfo, fullVirtPath, UString(), encrypted) == S_OK)
    {
      tempDirectory.DisableDeleting();
      RefreshListCtrl();
      return;
    }
  }

  CMyAutoPtr<CTmpProcessInfo> tmpProcessInfoPtr(new CTmpProcessInfo());
  CTmpProcessInfo *tmpProcessInfo = tmpProcessInfoPtr.get();
  tmpProcessInfo->FolderPath = tempDir;
  tmpProcessInfo->FilePath = tempFilePath;
  tmpProcessInfo->NeedDelete = true;
  tmpProcessInfo->UsePassword = usePassword;
  tmpProcessInfo->Password = password;

  if (!tmpProcessInfo->FileInfo.Find(tempFilePath))
    return;

  CTmpProcessInfoRelease tmpProcessInfoRelease(*tmpProcessInfo);

  if (!tryExternal)
    return;

  CProcess process;
  HRESULT res;
  if (editMode)
    res = StartEditApplication(tempFilePath, (HWND)*this, process);
  else
    res = StartApplication(tempDirNorm, tempFilePath, (HWND)*this, process);

  if ((HANDLE)process == 0)
    return;

  tmpProcessInfo->Window = (HWND)(*this);
  tmpProcessInfo->FullPathFolderPrefix = _currentFolderPrefix;
  tmpProcessInfo->ItemName = name;
  tmpProcessInfo->ProcessHandle = process.Detach();

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

  UINT64 currentFileTime;
  NTime::GetCurUtcFileTime(currentFileTime);
  UString searchWildCard = tempPath + kTempDirPrefix + L"*.tmp";
  searchWildCard += WCHAR(NName::kAnyStringWildcard);
  NFind::CEnumeratorW enumerator(searchWildCard);
  NFind::CFileInfoW fileInfo;
  while(enumerator.Next(fileInfo))
  {
    if (!fileInfo.IsDir())
      continue;
    const UINT64 &cTime = *(const UINT64 *)(&fileInfo.CTime);
    if(CheckDeleteItem(cTime, currentFileTime))
      RemoveDirectoryWithSubItems(tempPath + fileInfo.Name);
  }
}
*/
