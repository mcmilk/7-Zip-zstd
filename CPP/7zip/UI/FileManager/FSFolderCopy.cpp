// FSFolderCopy.cpp

#include "StdAfx.h"

#include <Winbase.h>

#include "../../../Common/Defs.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/Wildcard.h"

#include "../../../Windows/DLL.h"
#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileName.h"

#include "../../Common/FilePathAutoRename.h"

#include "FSFolder.h"

using namespace NWindows;
using namespace NFile;
using namespace NDir;
using namespace NName;
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

static bool FsCopyFile(CFSTR oldFile, CFSTR newFile, IProgress *progress, UInt64 &completedSize)
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
    
    IF_USE_MAIN_PATH_2(oldFile, newFile)
    {
      if (copyFunctionW == 0)
        return BOOLToBool(::CopyFileW(fs2us(oldFile), fs2us(newFile), TRUE));
      if (copyFunctionW(fs2us(oldFile), fs2us(newFile), CopyProgressRoutine,
          &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
    }
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString longPathExisting, longPathNew;
      if (!GetSuperPaths(oldFile, newFile, longPathExisting, longPathNew, USE_MAIN_PATH_2))
        return false;
      if (copyFunctionW(longPathExisting, longPathNew, CopyProgressRoutine,
          &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
    }
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
      if (copyFunction(fs2fas(oldFile), fs2fas(newFile),
          CopyProgressRoutine,&progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS))
        return true;
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }
    return BOOLToBool(::CopyFile(fs2fas(oldFile), fs2fas(newFile), TRUE));
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

#ifdef UNDER_CE
#define NON_CE_VAR(_v_)
#else
#define NON_CE_VAR(_v_) _v_
#endif

static bool FsMoveFile(CFSTR oldFile, CFSTR newFile,
    IProgress * NON_CE_VAR(progress),
    UInt64 & NON_CE_VAR(completedSize))
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
      IF_USE_MAIN_PATH_2(oldFile, newFile)
      {
        if (moveFunction(fs2us(oldFile), fs2us(newFile), CopyProgressRoutine,
            &progressInfo, MOVEFILE_COPY_ALLOWED))
          return true;
      }
      #ifdef WIN_LONG_PATH
      if ((!(USE_MAIN_PATH_2) || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED) && USE_SUPER_PATH_2)
      {
        UString longPathExisting, longPathNew;
        if (!GetSuperPaths(oldFile, newFile, longPathExisting, longPathNew, USE_MAIN_PATH_2))
          return false;
        if (moveFunction(longPathExisting, longPathNew, CopyProgressRoutine,
            &progressInfo, MOVEFILE_COPY_ALLOWED))
          return true;
      }
      #endif
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        return false;
    }
  // }
  // else
  #endif
    return MyMoveFile(oldFile, newFile);
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

static HRESULT FsCopyFile(
    const FString &srcPath,
    const CFileInfo &srcFileInfo,
    const FString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  FString destPath = destPathSpec;
  if (CompareFileNames(destPath, srcPath) == 0)
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
    if (!FsCopyFile(srcPath, destPathNew, callback, completedSize))
    {
      RINOK(SendMessageError(callback, NError::MyFormatMessage(GetLastError()) + L" : ", destPathNew));
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

static bool IsDestChild(const FString &src, const FString &dest)
{
  unsigned len = src.Len();
  if (dest.Len() < len)
    return false;
  if (dest.Len() != len && dest[len] != FCHAR_PATH_SEPARATOR)
    return false;
  return CompareFileNames(dest.Left(len), src) == 0;
}

static HRESULT CopyFolder(
    const FString &srcPath,
    const FString &destPath,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  RINOK(callback->SetCompleted(&completedSize));

  if (IsDestChild(srcPath, destPath))
  {
    RINOK(SendMessageError(callback, "can not copy folder onto itself: ", destPath));
    return E_ABORT;
  }

  if (!CreateComplexDir(destPath))
  {
    RINOK(SendMessageError(callback, "can not create folder: ", destPath));
    return E_ABORT;
  }
  CEnumerator enumerator(CombinePath(srcPath, FSTRING_ANY_MASK));
  CDirItem fi;
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
      RINOK(FsCopyFile(srcPath2, fi, destPath2, callback, completedSize));
    }
  }
  return S_OK;
}

/////////////////////////////////////////////////
// Move Operations

static HRESULT FsMoveFile(
    const FString &srcPath,
    const CFileInfo &srcFileInfo,
    const FString &destPath,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  if (CompareFileNames(destPath, srcPath) == 0)
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
    if (!FsMoveFile(srcPath, destPathNew, callback, completedSize))
    {
      RINOK(SendMessageError(callback, "can not move to file: ", destPathNew));
    }
  }
  completedSize += srcFileInfo.Size;
  RINOK(callback->SetCompleted(&completedSize));
  return S_OK;
}

static HRESULT FsMoveFolder(
    const FString &srcPath,
    const FString &destPath,
    IFolderOperationsExtractCallback *callback,
    UInt64 &completedSize)
{
  if (IsDestChild(srcPath, destPath))
  {
    RINOK(SendMessageError(callback, "can not move folder onto itself: ", destPath));
    return E_ABORT;
  }

  if (FsMoveFile(srcPath, destPath, callback, completedSize))
    return S_OK;

  if (!CreateComplexDir(destPath))
  {
    RINOK(SendMessageError(callback, "can not create folder: ", destPath));
    return E_ABORT;
  }
  {
    CEnumerator enumerator(CombinePath(srcPath, FSTRING_ANY_MASK));
    CDirItem fi;
    while (enumerator.Next(fi))
    {
      const FString srcPath2 = CombinePath(srcPath, fi.Name);
      const FString destPath2 = CombinePath(destPath, fi.Name);
      if (fi.IsDir())
      {
        RINOK(FsMoveFolder(srcPath2, destPath2, callback, completedSize));
      }
      else
      {
        RINOK(FsMoveFile(srcPath2, fi, destPath2, callback, completedSize));
      }
    }
  }
  if (!RemoveDir(srcPath))
  {
    RINOK(SendMessageError(callback, "can not remove folder: ", srcPath));
    return E_ABORT;
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyTo(Int32 moveMode, const UInt32 *indices, UInt32 numItems,
    Int32 /* includeAltStreams */, Int32 /* replaceAltStreamColon */,
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;

  CFsFolderStat stat;
  stat.Progress = callback;
  RINOK(GetItemsFullSize(indices, numItems, stat));
  RINOK(callback->SetTotal(stat.Size));
  RINOK(callback->SetNumFiles(stat.NumFiles));

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
  {
    // Does CreateComplexDir work in network ?
    if (!CreateComplexDir(destPath))
    {
      RINOK(SendMessageError(callback, "can not create folder: ", destPath));
      return E_ABORT;
    }
  }

  UInt64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (UInt32 i = 0; i < numItems; i++)
  {
    UInt32 index = indices[i];
    if (index >= (UInt32)Files.Size())
      continue;
    const CDirItem &fi = Files[index];
    FString destPath2 = destPath;
    if (!directName)
      destPath2 += fi.Name;
    FString srcPath;
    GetFullPath(fi, srcPath);
    if (fi.IsDir())
    {
      if (moveMode)
      {
        RINOK(FsMoveFolder(srcPath, destPath2, callback, completedSize));
      }
      else
      {
        RINOK(CopyFolder(srcPath, destPath2, callback, completedSize));
      }
    }
    else
    {
      if (moveMode)
      {
        RINOK(FsMoveFile(srcPath, fi, destPath2, callback, completedSize));
      }
      else
      {
        RINOK(FsCopyFile(srcPath, fi, destPath2, callback, completedSize));
      }
    }
  }
  return S_OK;
}

STDMETHODIMP CFSFolder::CopyFrom(Int32 /* moveMode */, const wchar_t * /* fromFolderPath */,
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
