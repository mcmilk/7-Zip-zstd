// FSFolderCopy.cpp

#include "StdAfx.h"

#include <Winbase.h>

#include "FSFolder.h"
#include "Windows/FileDir.h"
#include "Windows/Error.h"

#include "Common/StringConvert.h"

#include "../Common/FilePathAutoRename.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 

static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

static bool IsItWindows2000orHigher()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
      (versionInfo.dwMajorVersion >= 5);
}

struct CProgressInfo
{
  UINT64 StartPos;
  IProgress *Progress;
};

static DWORD CALLBACK CopyProgressRoutine(
  LARGE_INTEGER TotalFileSize,          // file size
  LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
  LARGE_INTEGER StreamSize,             // bytes in stream
  LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
  DWORD dwStreamNumber,                 // current stream
  DWORD dwCallbackReason,               // callback reason
  HANDLE hSourceFile,                   // handle to source file
  HANDLE hDestinationFile,              // handle to destination file
  LPVOID lpData                         // from CopyFileEx
)
{
  CProgressInfo &progressInfo = *(CProgressInfo *)lpData;
  UINT64 completed = progressInfo.StartPos + TotalBytesTransferred.QuadPart;
  if (progressInfo.Progress->SetCompleted(&completed) != S_OK)
    return PROGRESS_CANCEL;
  return PROGRESS_CONTINUE;
}

typedef BOOL (WINAPI * CopyFileExPointer)(
    IN LPCSTR lpExistingFileName,
    IN LPCSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags
    );

typedef BOOL (WINAPI * CopyFileExPointerW)(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags
    );

static bool MyCopyFile(LPCWSTR existingFile, LPCWSTR newFile,    
    IProgress *progress, UINT64 &completedSize)
{
  // if (IsItWindowsNT())
  // {
    CProgressInfo progressInfo;
    progressInfo.Progress = progress;
    progressInfo.StartPos = completedSize;
    BOOL CancelFlag = FALSE;
    CopyFileExPointerW copyFunctionW = (CopyFileExPointerW)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
        "CopyFileExW");
    if (copyFunctionW != 0)
    {
      if (copyFunctionW(existingFile, newFile, CopyProgressRoutine,
          &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }

    CopyFileExPointer copyFunction = (CopyFileExPointer)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
        "CopyFileExA");
    UINT codePage = GetCurrentCodePage();
    if (copyFunction != 0)
    {
      if (copyFunction(
          UnicodeStringToMultiByte(existingFile, codePage),
          UnicodeStringToMultiByte(newFile, codePage),
          CopyProgressRoutine,
          &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }
  // }
  return BOOLToBool(::CopyFile(
      GetSystemString(existingFile, codePage),
      GetSystemString(newFile, codePage),
      TRUE));
}

typedef BOOL (WINAPI * MoveFileWithProgressPointer)(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN DWORD dwFlags
    );

static bool MyMoveFile(LPCWSTR existingFile, LPCWSTR newFile,    
    IProgress *progress, UINT64 &completedSize)
{
  // if (IsItWindows2000orHigher())
  // {
    CProgressInfo progressInfo;
    progressInfo.Progress = progress;
    progressInfo.StartPos = completedSize;
    BOOL CancelFlag = FALSE;

    MoveFileWithProgressPointer moveFunction = (MoveFileWithProgressPointer)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
        "MoveFileWithProgressW");
    if (moveFunction != 0)
    {
      if (moveFunction(
          existingFile, newFile, CopyProgressRoutine,
          &progressInfo, MOVEFILE_COPY_ALLOWED))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }



  // }
  // else
    return NDirectory::MyMoveFile(existingFile, newFile);
}

static HRESULT MyCopyFile(
    const UString &srcPath,
    const CFileInfoW &srcFileInfo,
    const UString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  UString destPath = destPathSpec;
  if (destPath.CollateNoCase(srcPath) == 0)
  {
    UString message = UString(L"can not move file \'") + 
        GetUnicodeString(destPath, fileCodePage) + UString(L"\' onto itself");
    RINOK(callback->ShowMessage(message));
    return E_ABORT;
  }

  INT32 writeAskResult;
  CMyComBSTR destPathResult;
  RINOK(callback->AskWrite(
      GetUnicodeString(srcPath, fileCodePage),
      BoolToInt(false),
      &srcFileInfo.LastWriteTime, &srcFileInfo.Size, 
      GetUnicodeString(destPath, fileCodePage), 
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    UString destPathNew = UString(destPathResult);
    if (!::MyCopyFile(srcPath, destPathNew, callback, completedSize))
    {
      UString message = GetUnicodeString(NError::MyFormatMessage(GetLastError())) +
        UString(L" \'") + 
        GetUnicodeString(destPathNew, fileCodePage)+ 
        UString(L"\'");
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }
  completedSize += srcFileInfo.Size;
  return callback->SetCompleted(&completedSize);
}

static HRESULT CopyFolder(
    const UString &srcPath,
    const UString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  RINOK(callback->SetCompleted(&completedSize));

  UString destPath = destPathSpec;
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CollateNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == L'\\')
    {
      UString message = UString(L"can not copy folder \'") + 
          GetUnicodeString(destPath, fileCodePage) + UString(L"\' onto itself");
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    UString message = UString(L"can not create folder ") + 
        GetUnicodeString(destPath, fileCodePage);
    RINOK(callback->ShowMessage(message));
    return E_ABORT;
  }
  CEnumeratorW enumerator(srcPath + UString(L"\\*"));
  CFileInfoEx fileInfo;
  while (enumerator.Next(fileInfo))
  {
    const UString srcPath2 = srcPath + UString(L"\\") + fileInfo.Name;
    const UString destPath2 = destPath + UString(L"\\") + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RINOK(CopyFolder(srcPath2, destPath2,
        callback, fileCodePage, completedSize));
    }
    else
    {
      RINOK(MyCopyFile(srcPath2, fileInfo, destPath2,
          callback, fileCodePage, completedSize));
    }
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyTo(const UINT32 *indices, UINT32 numItems, 
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;
  UINT64 totalSize = 0;
  UINT32 i;
  for (i = 0; i < numItems; i++)
  {
    int index = indices[i];
    if (index >= _files.Size())
      return E_INVALIDARG;
    UINT64 size;
    RINOK(GetItemFullSize(indices[i], size, callback));
    totalSize += size;
  }

  callback->SetTotal(totalSize);
  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath[destPath.Length() - 1] != L'\\');
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
    /*
    // doesn't work in network
  else
    if (!NDirectory::CreateComplexDirectory(GetSystemString(destPath, _fileCodePage)))
    {
      DWORD lastError = ::GetLastError();
      UString message = UString(L"can not create folder ") + 
        destPath;
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }
    */

  UINT64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (i = 0; i < numItems; i++)
  {
    const CFileInfoW &fileInfo = _files[indices[i]];
    UString destPath2 = destPath;
    if (!directName)
      destPath2 += fileInfo.Name;
    UString srcPath = _path + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RINOK(CopyFolder(srcPath, destPath2, callback,
          _fileCodePage, completedSize));
    }
    else
    {
      RINOK(MyCopyFile(srcPath, fileInfo, destPath2,
          callback, _fileCodePage, completedSize));
    }
  }
  return S_OK;
}

/////////////////////////////////////////////////
// Move Operations

HRESULT MyMoveFile(
    const UString &srcPath,
    const CFileInfoW &srcFileInfo,
    const UString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  UString destPath = destPathSpec;
  if (destPath.CollateNoCase(srcPath) == 0)
  {
    UString message = UString(L"can not move file \'")
         + GetUnicodeString(destPath, fileCodePage) +
        UString(L"\' onto itself");
        RINOK(callback->ShowMessage(message));
    return E_ABORT;
  }

  INT32 writeAskResult;
  CMyComBSTR destPathResult;
  RINOK(callback->AskWrite(
      GetUnicodeString(srcPath, fileCodePage),
      BoolToInt(false),
      &srcFileInfo.LastWriteTime, &srcFileInfo.Size, 
      GetUnicodeString(destPath, fileCodePage), 
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    UString destPathNew = UString(destPathResult);
    if (!MyMoveFile(srcPath, destPathNew, callback, completedSize))
    {
      UString message = UString(L"can not move to file ") + 
          GetUnicodeString(destPathNew, fileCodePage);
      RINOK(callback->ShowMessage(message));
    }
  }
  completedSize += srcFileInfo.Size;
  RINOK(callback->SetCompleted(&completedSize));
  return S_OK;
}

HRESULT MyMoveFolder(
    const UString &srcPath,
    const UString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  UString destPath = destPathSpec;
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CollateNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == L'\\')
    {
      UString message = UString(L"can not move folder \'")
        + GetUnicodeString(destPath, fileCodePage) +
        UString(L"\' onto itself");
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }

  if (MyMoveFile(srcPath, destPath, callback, completedSize))
    return S_OK;

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    UString message = UString(L"can not create folder ") + 
        GetUnicodeString(destPath, fileCodePage);
    RINOK(callback->ShowMessage(message));
    return E_ABORT;
  }
  {
    CEnumeratorW enumerator(srcPath + UString(L"\\*"));
    CFileInfoEx fileInfo;
    while (enumerator.Next(fileInfo))
    {
      const UString srcPath2 = srcPath + UString(L"\\") + fileInfo.Name;
      const UString destPath2 = destPath + UString(L"\\") + fileInfo.Name;
      if (fileInfo.IsDirectory())
      {
        RINOK(MyMoveFolder(srcPath2, destPath2,
          callback, fileCodePage, completedSize));
      }
      else
      {
        RINOK(MyMoveFile(srcPath2, fileInfo, destPath2,
          callback, fileCodePage, completedSize));
      }
    }
  }
  if (!NDirectory::MyRemoveDirectory(srcPath))
  {
    UString message = UString(L"can not remove folder") + 
        GetUnicodeString(srcPath, fileCodePage);
    RINOK(callback->ShowMessage(message));
    return E_ABORT;
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::MoveTo(
    const UINT32 *indices, 
    UINT32 numItems, 
    const wchar_t *path,
    IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;

  UINT64 totalSize = 0;
  UINT32 i;
  for (i = 0; i < numItems; i++)
  {
    int index = indices[i];
    if (index >= _files.Size())
      return E_INVALIDARG;
    UINT64 size;
    RINOK(GetItemFullSize(indices[i], size, callback));
    totalSize += size;
  }
  callback->SetTotal(totalSize);

  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath[destPath.Length() - 1] != L'\\');
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
  else
    if (!NDirectory::CreateComplexDirectory(GetSystemString(destPath, _fileCodePage)))
    {
      UString message = UString(L"can not create folder ") + 
        destPath;
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }

  UINT64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (i = 0; i < numItems; i++)
  {
    const CFileInfoW &fileInfo = _files[indices[i]];
    UString destPath2 = destPath;
    if (!directName)
      destPath2 += fileInfo.Name;
    UString srcPath = _path + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RINOK(MyMoveFolder(srcPath, destPath2, callback,
          _fileCodePage, completedSize));
    }
    else
    {
      RINOK(MyMoveFile(srcPath, fileInfo, destPath2,
          callback, _fileCodePage, completedSize));
    }
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyFrom(
    const wchar_t *fromFolderPath,
    const wchar_t **itemsPaths, UINT32 numItems, IProgress *progress)
{
  return E_NOTIMPL;
}
  

