// FSFolderCopy.cpp

#include "StdAfx.h"

#include <Winbase.h>

#include "FSFolder.h"
#include "Windows/FileDir.h"
#include "Windows/Error.h"

#include "Common/StringConvert.h"

#include "Util/FilePathAutoRename.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

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
    IN LPCTSTR lpExistingFileName,
    IN LPCTSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags
    );

static bool MyCopyFile(LPCTSTR existingFile, LPCTSTR newFile,    
    IProgress *progress, UINT64 &completedSize)
{
  if (IsItWindowsNT())
  {
    CProgressInfo progressInfo;
    progressInfo.Progress = progress;
    progressInfo.StartPos = completedSize;
    BOOL CancelFlag = FALSE;
    CopyFileExPointer copyFunction = (CopyFileExPointer)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
    #ifdef UNICODE
        "CopyFileExW"
    #else
        "CopyFileExA"
    #endif
          );

    if (copyFunction == 0)
      return false;
    return BOOLToBool(copyFunction(existingFile, newFile, CopyProgressRoutine,
        &progressInfo, &CancelFlag, COPY_FILE_FAIL_IF_EXISTS));
  }
  else
    return BOOLToBool(::CopyFile(existingFile, newFile, TRUE));
}

typedef BOOL (WINAPI * MoveFileWithProgressPointer)(
    IN LPCTSTR lpExistingFileName,
    IN LPCTSTR lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN DWORD dwFlags
    );

static bool MyMoveFile(LPCTSTR existingFile, LPCTSTR newFile,    
    IProgress *progress, UINT64 &completedSize)
{
  if (IsItWindows2000orHigher())
  {
    CProgressInfo progressInfo;
    progressInfo.Progress = progress;
    progressInfo.StartPos = completedSize;
    BOOL CancelFlag = FALSE;

    MoveFileWithProgressPointer moveFunction = (MoveFileWithProgressPointer)
        ::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")),
    #ifdef UNICODE
        "MoveFileWithProgressW"
    #else
        "MoveFileWithProgressA"
    #endif
      );
    if (moveFunction == 0)
      return false;
    return BOOLToBool(moveFunction(
        existingFile, newFile, CopyProgressRoutine,
        &progressInfo, MOVEFILE_COPY_ALLOWED));
  }
  else
    return BOOLToBool(::MoveFile(existingFile, newFile));
}

static HRESULT MyCopyFile(
    const CSysString &srcPath,
    const CFileInfo &srcFileInfo,
    const CSysString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  CSysString destPath = destPathSpec;
  if (destPath.CollateNoCase(srcPath) == 0)
  {
    UString message = UString(L"can not move file \'") + 
        GetUnicodeString(destPath, fileCodePage) + UString(L"\' onto itself");
    RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
    return E_ABORT;
  }

  INT32 writeAskResult;
  CComBSTR destPathResult;
  RETURN_IF_NOT_S_OK(callback->AskWrite(
      GetUnicodeString(srcPath, fileCodePage),
      BoolToInt(false),
      &srcFileInfo.LastWriteTime, &srcFileInfo.Size, 
      GetUnicodeString(destPath, fileCodePage), 
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    CSysString destPathNew = GetSystemString(UString(destPathResult), fileCodePage);
    if (!::MyCopyFile(srcPath, destPathNew, callback, completedSize))
    {
      UString message = GetUnicodeString(NError::MyFormatMessage(GetLastError())) +
        UString(L" \'") + 
        GetUnicodeString(destPathNew, fileCodePage)+ 
        UString(L"\'");
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }
  completedSize += srcFileInfo.Size;
  return callback->SetCompleted(&completedSize);
}

HRESULT CopyFolder(
    const CSysString &srcPath,
    const CSysString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  RETURN_IF_NOT_S_OK(callback->SetCompleted(&completedSize));

  CSysString destPath = destPathSpec;
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CollateNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == TEXT('\\'))
    {
      UString message = UString(L"can not copy folder \'") + 
          GetUnicodeString(destPath, fileCodePage) + UString(L"\' onto itself");
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    UString message = UString(L"can not create folder ") + 
        GetUnicodeString(destPath, fileCodePage);
    RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
    return E_ABORT;
  }
  CEnumerator enumerator(srcPath + CSysString(TEXT("\\*")));
  CFileInfoEx fileInfo;
  while (enumerator.Next(fileInfo))
  {
    const CSysString aSrcPath2 = srcPath + CSysString(TEXT("\\")) + fileInfo.Name;
    const CSysString destPath2 = destPath + CSysString(TEXT("\\")) + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(CopyFolder(aSrcPath2, destPath2,
        callback, fileCodePage, completedSize));
    }
    else
    {
      RETURN_IF_NOT_S_OK(MyCopyFile(aSrcPath2, fileInfo, destPath2,
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
    RETURN_IF_NOT_S_OK(GetItemFullSize(indices[i], size, callback));
    totalSize += size;
  }

  callback->SetTotal(totalSize);
  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool aDirectName = (destPath[destPath.Length() - 1] != L'\\');
  if (aDirectName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
  else
    if (!NDirectory::CreateComplexDirectory(GetSystemString(destPath, _fileCodePage)))
    {
      UString message = UString(L"can not create folder ") + 
        destPath;
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
      return E_ABORT;
    }

  UINT64 completedSize = 0;
  RETURN_IF_NOT_S_OK(callback->SetCompleted(&completedSize));
  for (i = 0; i < numItems; i++)
  {
    const CFileInfo &fileInfo = _files[indices[i]];
    CSysString destPath2 = GetSystemString(destPath);
    if (!aDirectName)
      destPath2 += fileInfo.Name;
    CSysString srcPath = _path + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(CopyFolder(srcPath, destPath2, callback,
          _fileCodePage, completedSize));
    }
    else
    {
      RETURN_IF_NOT_S_OK(MyCopyFile(srcPath, fileInfo, destPath2,
          callback, _fileCodePage, completedSize));
    }
  }
  return S_OK;
}

/////////////////////////////////////////////////
// Move Operations

HRESULT MyMoveFile(
    const CSysString &srcPath,
    const CFileInfo &srcFileInfo,
    const CSysString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  CSysString destPath = destPathSpec;
  if (destPath.CollateNoCase(srcPath) == 0)
  {
    UString message = UString(L"can not move file \'")
         + GetUnicodeString(destPath, fileCodePage) +
        UString(L"\' onto itself");
        RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
    return E_ABORT;
  }

  INT32 writeAskResult;
  CComBSTR destPathResult;
  RETURN_IF_NOT_S_OK(callback->AskWrite(
      GetUnicodeString(srcPath, fileCodePage),
      BoolToInt(false),
      &srcFileInfo.LastWriteTime, &srcFileInfo.Size, 
      GetUnicodeString(destPath, fileCodePage), 
      &destPathResult,
      &writeAskResult));
  if (IntToBool(writeAskResult))
  {
    CSysString destPathNew = GetSystemString(UString(destPathResult), fileCodePage);
    if (!MyMoveFile(srcPath, destPathNew, callback, completedSize))
    {
      UString message = UString(L"can not move to file ") + 
          GetUnicodeString(destPathNew, fileCodePage);
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
    }
  }
  completedSize += srcFileInfo.Size;
  RETURN_IF_NOT_S_OK(callback->SetCompleted(&completedSize));
  return S_OK;
}

HRESULT MyMoveFolder(
    const CSysString &srcPath,
    const CSysString &destPathSpec,
    IFolderOperationsExtractCallback *callback,
    UINT fileCodePage,
    UINT64 &completedSize)
{
  CSysString destPath = destPathSpec;
  int len = srcPath.Length();
  if (destPath.Length() >= len && srcPath.CollateNoCase(destPath.Left(len)) == 0)
  {
    if (destPath.Length() == len || destPath[len] == TEXT('\\'))
    {
      UString message = UString(L"can not move folder \'")
        + GetUnicodeString(destPath, fileCodePage) +
        UString(L"\' onto itself");
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
      return E_ABORT;
    }
  }

  if (MyMoveFile(srcPath, destPath, callback, completedSize))
    return S_OK;

  if (!NDirectory::CreateComplexDirectory(destPath))
  {
    UString message = UString(L"can not create folder ") + 
        GetUnicodeString(destPath, fileCodePage);
    RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
    return E_ABORT;
  }
  {
    CEnumerator enumerator(srcPath + CSysString(TEXT("\\*")));
    CFileInfoEx fileInfo;
    while (enumerator.Next(fileInfo))
    {
      const CSysString aSrcPath2 = srcPath + CSysString(TEXT("\\")) + fileInfo.Name;
      const CSysString destPath2 = destPath + CSysString(TEXT("\\")) + fileInfo.Name;
      if (fileInfo.IsDirectory())
      {
        RETURN_IF_NOT_S_OK(MyMoveFolder(aSrcPath2, destPath2,
          callback, fileCodePage, completedSize));
      }
      else
      {
        RETURN_IF_NOT_S_OK(MyMoveFile(aSrcPath2, fileInfo, destPath2,
          callback, fileCodePage, completedSize));
      }
    }
  }
  if (!::RemoveDirectory(srcPath))
  {
    UString message = UString(L"can not remove folder") + 
        GetUnicodeString(srcPath, fileCodePage);
    RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
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
    RETURN_IF_NOT_S_OK(GetItemFullSize(indices[i], size, callback));
    totalSize += size;
  }
  callback->SetTotal(totalSize);

  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool aDirectName = (destPath[destPath.Length() - 1] != L'\\');
  if (aDirectName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }
  else
    if (!NDirectory::CreateComplexDirectory(GetSystemString(destPath, _fileCodePage)))
    {
      UString message = UString(L"can not create folder ") + 
        destPath;
      RETURN_IF_NOT_S_OK(callback->ShowMessage(message));
      return E_ABORT;
    }

  UINT64 completedSize = 0;
  RETURN_IF_NOT_S_OK(callback->SetCompleted(&completedSize));
  for (i = 0; i < numItems; i++)
  {
    const CFileInfo &fileInfo = _files[indices[i]];
    CSysString destPath2 = GetSystemString(destPath);
    if (!aDirectName)
      destPath2 += fileInfo.Name;
    CSysString srcPath = _path + fileInfo.Name;
    if (fileInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(MyMoveFolder(srcPath, destPath2, callback,
          _fileCodePage, completedSize));
    }
    else
    {
      RETURN_IF_NOT_S_OK(MyMoveFile(srcPath, fileInfo, destPath2,
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
  

