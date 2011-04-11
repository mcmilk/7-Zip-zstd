// FSFolderCopy.cpp

#include "StdAfx.h"

#include <Winbase.h>

#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/Error.h"
#include "Windows/FileDir.h"

#include "../../Common/FilePathAutoRename.h"

#include "FSFolder.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NFsFolder {

/*
static bool IsItWindows2000orHigher()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo))
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
      (versionInfo.dwMajorVersion >= 5);
}
*/

struct CProgressInfo
{
  UInt64 StartPos;
  IProgress *Progress;
};

#ifndef PROGRESS_CONTINUE

#define PROGRESS_CONTINUE 0
#define PROGRESS_CANCEL 1

#define COPY_FILE_FAIL_IF_EXISTS 0x00000001

typedef
DWORD
(WINAPI* LPPROGRESS_ROUTINE)(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData
    );

#endif

static DWORD CALLBACK CopyProgressRoutine(
  LARGE_INTEGER /* TotalFileSize */,          // file size
  LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
  LARGE_INTEGER /* StreamSize */,             // bytes in stream
  LARGE_INTEGER /* StreamBytesTransferred */, // bytes transferred for stream
  DWORD /* dwStreamNumber */,                 // current stream
  DWORD /* dwCallbackReason */,               // callback reason
  HANDLE /* hSourceFile */,                   // handle to source file
  HANDLE /* hDestinationFile */,              // handle to destination file
  LPVOID lpData                         // from CopyFileEx
)
{
  CProgressInfo &progressInfo = *(CProgressInfo *)lpData;
  UInt64 completed = progressInfo.StartPos + TotalBytesTransferred.QuadPart;
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

static bool MyCopyFile(CFSTR existingFile, CFSTR newFile, IProgress *progress, UInt64 &completedSize)
{
  CProgressInfo progressInfo;
  progressInfo.Progress = progress;
  progressInfo.StartPos = completedSize;
  BOOL CancelFlag = FALSE;
  #ifndef _UNICODE
  if (g_IsNT)
  #endif
  {
    const wchar_t *k_DllName =
        #ifdef UNDER_CE
        L"coredll.dll"
        #else
        L"kernel32.dll"
        #endif
        ;
    CopyFileExPointerW copyFunctionW = (CopyFileExPointerW)
        My_GetProcAddress(::GetModuleHandleW(k_DllName), "CopyFileExW");
    if (copyFunctionW == 0)
      return BOOLToBool(::CopyFileW(fs2us(existingFile), fs2us(newFile), TRUE));
    if (copyFunctionW(fs2us(existingFile), fs2us(newFile), CopyProgressRoutine,
        &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
      return true;
    #ifdef WIN_LONG_PATH
    UString longPathExisting, longPathNew;
    if (!NDirectory::GetLongPaths(existingFile, newFile, longPathExisting, longPathNew))
      return false;
    if (copyFunctionW(longPathExisting, longPathNew, CopyProgressRoutine,
        &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
      return true;
    #endif
    return false;
  }
  #ifndef _UNICODE
  else
  {
    CopyFileExPointer copyFunction = (CopyFileExPointer)
        ::GetProcAddress(::GetModuleHandleA("kernel32.dll"),
        "CopyFileExA");
    if (copyFunction != 0)
    {
      if (copyFunction(fs2fas(existingFile), fs2fas(newFile),
          CopyProgressRoutine,&progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }
    return BOOLToBool(::CopyFile(fs2fas(existingFile), fs2fas(newFile), TRUE));
  }
  #endif
}

typedef BOOL (WINAPI * MoveFileWithProgressPointer)(
    IN LPCWSTR lpExistingFileName,
    IN LPCWSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN DWORD dwFlags
    );

static bool MyMoveFile(CFSTR existingFile, CFSTR newFile, IProgress *progress, UInt64 &completedSize)
{
  #ifndef UNDER_CE
  // if (IsItWindows2000orHigher())
  // {
    CProgressInfo progressInfo;
    progressInfo.Progress = progress;
    progressInfo.StartPos = completedSize;

    MoveFileWithProgressPointer moveFunction = (MoveFileWithProgressPointer)
        My_GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
        "MoveFileWithProgressW");
    if (moveFunction != 0)
    {
      if (moveFunction(
          fs2us(existingFile), fs2us(newFile), CopyProgressRoutine,
          &progressInfo, MOVEFILE_COPY_ALLOWED))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      {
        #ifdef WIN_LONG_PATH
        UString longPathExisting, longPathNew;
        if (!NDirectory::GetLongPaths(existingFile, newFile, longPathExisting, longPathNew))
          return false;
        if (moveFunction(longPathExisting, longPathNew, CopyProgressRoutine,
            &progressInfo, MOVEFILE_COPY_ALLOWED))
          return true;
        #endif
        if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
          return false;
      }
    }
  // }
  // else
  #endif
    return NDirectory::MyMoveFile(existingFile, newFile);
}

static HRESULT SendMessageError(IFolderOperationsExtractCallback *callback,
    const wchar_t *message, const FString &fileName)
{
  return callback->ShowMessage(message + fs2us(fileName));
}

static HRESULT SendMessageError(IFolderOperationsExtractCallback *callback,
    const char *message, const FString &fileName)
{
  return SendMessageError(callback, MultiByteToUnicodeString(message), fileName);
}

static HRESULT MyCopyFile(
    const FString &srcPath,
    const CFileInfo &srcFileInfo,
    const FString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  FString destPath = destPathSpec;
  if (destPath.CompareNoCase(srcPath) == 0)
  {
    RINOK(SendMessageError(callback, "can not copy file onto itself: ", destPath));
    return E_ABORT;
  }

  Int32 writeAskResult;
  CMyComBSTR destPathResult;
  RINOK(callback->AskWrite(
      fs2us(srcPath),
      BoolToInt(false),
      &srcFileInfo.MTime, &srcFileInfo.Size,
      fs2us(destPath),
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    FString destPathNew = us2fs(destPathResult);
    RINOK(callback->SetCurrentFilePath(fs2us(srcPath)));
    if (!MyCopyFile(srcPath, destPathNew, callback, completedSize))
    {
      RINOK(SendMessageError(callback, NError::MyFormatMessageW(GetLastError()) + L" : ", destPathNew));
      return E_ABORT;
    }
  }
  completedSize += srcFileInfo.Size;
  return callback->SetCompleted(&completedSize);
}

static FString CombinePath(const FString &folderPath, const FString &fileName)
{
  return folderPath + FCHAR_PATH_SEPARATOR + fileName;
}

static HRESULT CopyFolder(
    const FString &srcPath,
    const FString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  RINOK(callback->SetCompleted(&completedSize));

  const FString destPath = destPathSpec;
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CompareNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == FCHAR_PATH_SEPARATOR)
    {
      RINOK(SendMessageError(callback, "can not copy folder onto itself: ", destPath));
      return E_ABORT;
    }
  }

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    RINOK(SendMessageError(callback, "can not create folder: ", destPath));
    return E_ABORT;
  }
  CEnumerator enumerator(CombinePath(srcPath, FSTRING_ANY_MASK));
  CFileInfoEx fi;
  while (enumerator.Next(fi))
  {
    const FString srcPath2 = CombinePath(srcPath, fi.Name);
    const FString destPath2 = CombinePath(destPath, fi.Name);
    if (fi.IsDir())
    {
      RINOK(CopyFolder(srcPath2, destPath2, callback, completedSize))
    }
    else
    {
      RINOK(MyCopyFile(srcPath2, fi, destPath2, callback, completedSize));
    }
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyTo(const UInt32 *indices, UInt32 numItems,
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;
  
  UInt64 numFolders, numFiles, totalSize;
  GetItemsFullSize(indices, numItems, numFolders, numFiles, totalSize, callback);
  RINOK(callback->SetTotal(totalSize));
  RINOK(callback->SetNumFiles(numFiles));
  
  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath.Back() != WCHAR_PATH_SEPARATOR);
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
    /*
    // doesn't work in network
  else
    if (!NDirectory::CreateComplexDirectory(destPath)))
    {
      DWORD lastError = ::GetLastError();
      UString message = UString(L"can not create folder ") +
        destPath;
      RINOK(callback->ShowMessage(message));
      return E_ABORT;
    }
    */

  UInt64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (UInt32 i = 0; i < numItems; i++)
  {
    const CDirItem &fi = *_refs[indices[i]];
    FString destPath2 = us2fs(destPath);
    if (!directName)
      destPath2 += fi.Name;
    FString srcPath = _path + GetPrefix(fi) + fi.Name;
    if (fi.IsDir())
    {
      RINOK(CopyFolder(srcPath, destPath2, callback, completedSize));
    }
    else
    {
      RINOK(MyCopyFile(srcPath, fi, destPath2, callback, completedSize));
    }
  }
  return S_OK;
}

/////////////////////////////////////////////////
// Move Operations

static HRESULT MyMoveFile(
    const FString &srcPath,
    const CFileInfo &srcFileInfo,
    const FString &destPath,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  if (destPath.CompareNoCase(srcPath) == 0)
  {
    RINOK(SendMessageError(callback, "can not move file onto itself: ", srcPath));
    return E_ABORT;
  }

  Int32 writeAskResult;
  CMyComBSTR destPathResult;
  RINOK(callback->AskWrite(
      fs2us(srcPath),
      BoolToInt(false),
      &srcFileInfo.MTime, &srcFileInfo.Size,
      fs2us(destPath),
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    FString destPathNew = us2fs(destPathResult);
    RINOK(callback->SetCurrentFilePath(fs2us(srcPath)));
    if (!MyMoveFile(srcPath, destPathNew, callback, completedSize))
    {
      RINOK(SendMessageError(callback, "can not move to file: ", destPathNew));
    }
  }
  completedSize += srcFileInfo.Size;
  RINOK(callback->SetCompleted(&completedSize));
  return S_OK;
}

static HRESULT MyMoveFolder(
    const FString &srcPath,
    const FString &destPath,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CompareNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == FCHAR_PATH_SEPARATOR)
    {
      RINOK(SendMessageError(callback, "can not move folder onto itself: ", destPath));
      return E_ABORT;
    }
  }

  if (MyMoveFile(srcPath, destPath, callback, completedSize))
    return S_OK;

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    RINOK(SendMessageError(callback, "can not create folder: ", destPath));
    return E_ABORT;
  }
  {
    CEnumerator enumerator(CombinePath(srcPath, FSTRING_ANY_MASK));
    CFileInfoEx fi;
    while (enumerator.Next(fi))
    {
      const FString srcPath2 = CombinePath(srcPath, fi.Name);
      const FString destPath2 = CombinePath(destPath, fi.Name);
      if (fi.IsDir())
      {
        RINOK(MyMoveFolder(srcPath2, destPath2, callback, completedSize));
      }
      else
      {
        RINOK(MyMoveFile(srcPath2, fi, destPath2, callback, completedSize));
      }
    }
  }
  if (!NDirectory::MyRemoveDirectory(srcPath))
  {
    RINOK(SendMessageError(callback, "can not remove folder: ", srcPath));
    return E_ABORT;
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::MoveTo(
    const UInt32 *indices,
    UInt32 numItems,
    const wchar_t *path,
    IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;

  UInt64 numFolders, numFiles, totalSize;
  GetItemsFullSize(indices, numItems, numFolders, numFiles, totalSize, callback);
  RINOK(callback->SetTotal(totalSize));
  RINOK(callback->SetNumFiles(numFiles));

  FString destPath = us2fs(path);
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath.Back() != FCHAR_PATH_SEPARATOR);
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
  else
    if (!NDirectory::CreateComplexDirectory(destPath))
    {
      RINOK(SendMessageError(callback, "can not create folder: ", destPath));
      return E_ABORT;
    }

  UInt64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (UInt32 i = 0; i < numItems; i++)
  {
    const CDirItem &fi = *_refs[indices[i]];
    FString destPath2 = destPath;
    if (!directName)
      destPath2 += fi.Name;
    FString srcPath = _path + GetPrefix(fi) + fi.Name;
    if (fi.IsDir())
    {
      RINOK(MyMoveFolder(srcPath, destPath2, callback, completedSize));
    }
    else
    {
      RINOK(MyMoveFile(srcPath, fi, destPath2, callback, completedSize));
    }
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyFrom(const wchar_t * /* fromFolderPath */,
    const wchar_t ** /* itemsPaths */, UInt32 /* numItems */, IProgress * /* progress */)
{
  /*
  UInt64 numFolders, numFiles, totalSize;
  numFiles = numFolders = totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    UString path = (UString)fromFolderPath + itemsPaths[i];

    CFileInfo fi;
    if (!FindFile(path, fi))
      return ::GetLastError();
    if (fi.IsDir())
    {
      UInt64 subFolders, subFiles, subSize;
      RINOK(GetFolderSize(CombinePath(path, fi.Name), subFolders, subFiles, subSize, progress));
      numFolders += subFolders;
      numFolders++;
      numFiles += subFiles;
      totalSize += subSize;
    }
    else
    {
      numFiles++;
      totalSize += fi.Size;
    }
  }
  RINOK(progress->SetTotal(totalSize));
  RINOK(callback->SetNumFiles(numFiles));
  for (i = 0; i < numItems; i++)
  {
    UString path = (UString)fromFolderPath + itemsPaths[i];
  }
  return S_OK;
  */
  return E_NOTIMPL;
}

STDMETHODIMP CFSFolder::CopyFromFile(UInt32 /* index */, const wchar_t * /* fullFilePath */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

}
