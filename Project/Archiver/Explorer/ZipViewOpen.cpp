// ZipViewOpen.cpp

#include "StdAfx.h"

#include "ZipViewObject.h"

#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Synchronization.h"

#include "Common/Random.h"
#include "Common/StringConvert.h"
#include "MyIDList.h"
#include "Windows/Thread.h"

using namespace NWindows;
using namespace NSynchronization;
using namespace NFile;
using namespace NDirectory;

static LPCTSTR kTempDirPrefix = _T("7zOp"); 
static const UINT32 kTempDirNameSize = 8; 

static CSysString GetRandomTempDir()
{
  CSysString aTempPath;
  if(!NFile::NDirectory::MyGetTempPath(aTempPath))
    throw 171949;
  CSysString aPrefix = aTempPath + kTempDirPrefix;
  CRandom aRandom;
  aRandom.Init();
  CSysString aDirName;
  while(true)
  {
    UINT32 aRandomNumber = aRandom.Generate();
    TCHAR aRandomNumberString[32];
    _stprintf(aRandomNumberString, _T("%04X"), aRandomNumber);
    aDirName = aPrefix + aRandomNumberString;
    if(!NFile::NFind::DoesFileExist(aDirName))
      break;
  }
  bool aResult = NFile::NDirectory::MyCreateDirectory(aDirName);
  return aDirName;
}

class CExitEventLauncher
{
public:
  CManualResetEvent m_ExitEvent;
  CExitEventLauncher(): m_ExitEvent(false) {};
  ~CExitEventLauncher() {  m_ExitEvent.Set(); }
} g_ExitEventLauncher;


struct CTmpProcessInfo
{
  CSysString FolderName;
  CSysString FileName;
  HANDLE ProcessHandle;
};

class CTmpProcessInfoList
{
public:
  CObjectVector<CTmpProcessInfo> m_Items;
} g_TmpProcessInfoList;

static DWORD WINAPI MyThreadFunction(void *aParam)
{
  CTmpProcessInfo *aTmpProcessInfo = (CTmpProcessInfo *)aParam;
  HANDLE hProcess = aTmpProcessInfo->ProcessHandle;
  HANDLE anEvents[2] = { g_ExitEventLauncher.m_ExitEvent, hProcess};
  DWORD anWaitResult = ::WaitForMultipleObjects(2, anEvents, FALSE, INFINITE);
  ::CloseHandle(hProcess);
  if (anWaitResult == WAIT_OBJECT_0 + 0)
    return 0;
  if (anWaitResult != WAIT_OBJECT_0 + 1)
    return 1;
  // RemoveDirectoryWithSubItems(aTmpProcessInfo->FolderName);
  Sleep(500);
  DeleteFileAlways(aTmpProcessInfo->FileName);
  ::RemoveDirectory(aTmpProcessInfo->FolderName);
  return 0;
}

static CCriticalSection g_CriticalSection;

void CZipViewObject::OpenItem(UINT32 anIndex)
{
  NExtractionDialog::CModeInfo anExtractModeInfo;
  anExtractModeInfo.OverwriteMode = NExtractionDialog::NOverwriteMode::kWithoutPrompt;
  anExtractModeInfo.PathMode = NExtractionDialog::NPathMode::kCurrentPathnames;
  anExtractModeInfo.FilesMode = NExtractionDialog::NFilesMode::kSelected;
  
  CSysString aTempDir = GetRandomTempDir();
  CRecordVector<UINT32> anIndexes;
  anIndexes.Add(anIndex);
  ExtractItems(anExtractModeInfo, aTempDir, anIndexes, false, L"");

  CSysString aTempFileName = aTempDir + NFile::NName::kDirDelimiter +
      GetSystemString(GetNameOfObject(m_ArchiveFolder, anIndex));
  SHELLEXECUTEINFO anExecInfo;
  anExecInfo.cbSize = sizeof(anExecInfo);
  anExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // 
  anExecInfo.hwnd = m_hWndParent;
  anExecInfo.lpVerb = NULL;
  anExecInfo.lpFile = aTempFileName;
  anExecInfo.lpParameters = NULL;
  anExecInfo.lpDirectory = NULL;
  anExecInfo.nShow = SW_SHOWNORMAL;
  anExecInfo.hProcess = 0;
  
  bool anSuccess = BOOLToBool(::ShellExecuteEx(&anExecInfo));
  UINT32 aResult = (UINT32)anExecInfo.hInstApp;
  if(aResult <= 32)
  {
    switch(aResult)
    {
      case SE_ERR_NOASSOC:
      {
        MyMessageBox(IDS_ERROR_NO_ASSOCIATION, 0x02000607);
        DeleteFileAlways(aTempFileName);
        ::RemoveDirectory(aTempDir);
        return;
      }
    }
  }

  if (anExecInfo.hProcess == 0)
    return;

  CTmpProcessInfo aTmpProcessInfo;
  aTmpProcessInfo.FolderName = aTempDir;
  aTmpProcessInfo.FileName = aTempFileName;
  aTmpProcessInfo.ProcessHandle = anExecInfo.hProcess;
  {
    NSynchronization::CSingleLock aLock(&g_CriticalSection, true);
    g_TmpProcessInfoList.m_Items.Add(aTmpProcessInfo);
  }
  
  CThread aThread;
  if (!aThread.Create(MyThreadFunction, &g_TmpProcessInfoList.m_Items.Back()))
    throw 271824;
}

static const UINT64 kTimeLimit = UINT64(10000000) * 3600 * 24;

static bool CheckDeleteItem(UINT64 aCurrentFileTime, UINT64 aFolderFileTime)
{
  return (aCurrentFileTime - aFolderFileTime > kTimeLimit &&
      aFolderFileTime - aCurrentFileTime > kTimeLimit);
}

void DeleteOldTempFiles()
{
  CSysString aTempPath;
  if(!NFile::NDirectory::MyGetTempPath(aTempPath))
    throw 1;

  SYSTEMTIME aSystemTime;
  ::GetSystemTime(&aSystemTime);
  UINT64 aCurrentFileTime;
  if(!::SystemTimeToFileTime(&aSystemTime, (FILETIME *)&aCurrentFileTime))
    throw 2;
  CSysString aSearchWildCard = aTempPath + kTempDirPrefix;
  aSearchWildCard += TCHAR(NName::kAnyStringWildcard);
  NFind::CEnumerator anEnumerator(aSearchWildCard);
  NFind::CFileInfo aFileInfo;
  while(anEnumerator.Next(aFileInfo))
  {
    if (!aFileInfo.IsDirectory())
      continue;
    if (aFileInfo.Name.Length() != kTempDirNameSize)
      continue;
    const UINT64 &aCreationTime = *(const UINT64 *)(&aFileInfo.CreationTime);
    if(CheckDeleteItem(aCreationTime, aCurrentFileTime))
      RemoveDirectoryWithSubItems(aTempPath + aFileInfo.Name);
  }
}
