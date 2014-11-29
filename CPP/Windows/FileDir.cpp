// Windows/FileDir.cpp

#include "StdAfx.h"

#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

#include "FileDir.h"
#include "FileFind.h"
#include "FileName.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;

namespace NWindows {
namespace NFile {
namespace NDir {

#ifndef UNDER_CE

bool GetWindowsDir(FString &path)
{
  UINT needLength;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetWindowsDirectory(s, MAX_PATH + 1);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetWindowsDirectoryW(s, MAX_PATH + 1);
    path = us2fs(s);
  }
  return (needLength > 0 && needLength <= MAX_PATH);
}

bool GetSystemDir(FString &path)
{
  UINT needLength;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetSystemDirectory(s, MAX_PATH + 1);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetSystemDirectoryW(s, MAX_PATH + 1);
    path = us2fs(s);
  }
  return (needLength > 0 && needLength <= MAX_PATH);
}
#endif

bool SetDirTime(CFSTR path, const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
  #endif
  
  HANDLE hDir = INVALID_HANDLE_VALUE;
  IF_USE_MAIN_PATH
    hDir = ::CreateFileW(fs2us(path), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  #ifdef WIN_LONG_PATH
  if (hDir == INVALID_HANDLE_VALUE && USE_SUPER_PATH)
  {
    UString longPath;
    if (GetSuperPath(path, longPath, USE_MAIN_PATH))
      hDir = ::CreateFileW(longPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
          NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  }
  #endif

  bool res = false;
  if (hDir != INVALID_HANDLE_VALUE)
  {
    res = BOOLToBool(::SetFileTime(hDir, cTime, aTime, mTime));
    ::CloseHandle(hDir);
  }
  return res;
}

bool SetFileAttrib(CFSTR path, DWORD attrib)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::SetFileAttributes(fs2fas(path), attrib))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::SetFileAttributesW(fs2us(path), attrib))
        return true;
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        return BOOLToBool(::SetFileAttributesW(longPath, attrib));
    }
    #endif
  }
  return false;
}

bool RemoveDir(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::RemoveDirectory(fs2fas(path)))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::RemoveDirectoryW(fs2us(path)))
        return true;
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        return BOOLToBool(::RemoveDirectoryW(longPath));
    }
    #endif
  }
  return false;
}

bool MyMoveFile(CFSTR oldFile, CFSTR newFile)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::MoveFile(fs2fas(oldFile), fs2fas(newFile)))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH_2(oldFile, newFile)
      if (::MoveFileW(fs2us(oldFile), fs2us(newFile)))
        return true;
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString d1, d2;
      if (GetSuperPaths(oldFile, newFile, d1, d2, USE_MAIN_PATH_2))
        return BOOLToBool(::MoveFileW(d1, d2));
    }
    #endif
  }
  return false;
}

#ifndef UNDER_CE

EXTERN_C_BEGIN
typedef BOOL (WINAPI *Func_CreateHardLinkW)(
    LPCWSTR lpFileName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
EXTERN_C_END

bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
    /*
    if (::CreateHardLink(fs2fas(newFileName), fs2fas(existFileName), NULL))
      return true;
    */
  }
  else
  #endif
  {
    Func_CreateHardLinkW my_CreateHardLinkW = (Func_CreateHardLinkW)
        ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "CreateHardLinkW");
    if (!my_CreateHardLinkW)
      return false;
    IF_USE_MAIN_PATH_2(newFileName, existFileName)
      if (my_CreateHardLinkW(fs2us(newFileName), fs2us(existFileName), NULL))
        return true;
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH_2)
    {
      UString d1, d2;
      if (GetSuperPaths(newFileName, existFileName, d1, d2, USE_MAIN_PATH_2))
        return BOOLToBool(my_CreateHardLinkW(d1, d2, NULL));
    }
    #endif
  }
  return false;
}

#endif

bool CreateDir(CFSTR path)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::CreateDirectory(fs2fas(path), NULL))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::CreateDirectoryW(fs2us(path), NULL))
        return true;
    #ifdef WIN_LONG_PATH
    if ((!USE_MAIN_PATH || ::GetLastError() != ERROR_ALREADY_EXISTS) && USE_SUPER_PATH)
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        return BOOLToBool(::CreateDirectoryW(longPath, NULL));
    }
    #endif
  }
  return false;
}

bool CreateComplexDir(CFSTR _aPathName)
{
  FString pathName = _aPathName;
  int pos = pathName.ReverseFind(FCHAR_PATH_SEPARATOR);
  if (pos > 0 && (unsigned)pos == pathName.Len() - 1)
  {
    if (pathName.Len() == 3 && pathName[1] == L':')
      return true; // Disk folder;
    pathName.Delete(pos);
  }
  const FString pathName2 = pathName;
  pos = pathName.Len();
  
  for (;;)
  {
    if (CreateDir(pathName))
      break;
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
    {
      NFind::CFileInfo fileInfo;
      if (!fileInfo.Find(pathName)) // For network folders
        return true;
      if (!fileInfo.IsDir())
        return false;
      break;
    }
    pos = pathName.ReverseFind(FCHAR_PATH_SEPARATOR);
    if (pos < 0 || pos == 0)
      return false;
    if (pathName[pos - 1] == L':')
      return false;
    pathName.DeleteFrom(pos);
  }
  
  while (pos < (int)pathName2.Len())
  {
    pos = pathName2.Find(FCHAR_PATH_SEPARATOR, pos + 1);
    if (pos < 0)
      pos = pathName2.Len();
    pathName.SetFrom(pathName2, pos);
    if (!CreateDir(pathName))
      return false;
  }
  
  return true;
}

bool DeleteFileAlways(CFSTR path)
{
  if (!SetFileAttrib(path, 0))
    return false;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    if (::DeleteFile(fs2fas(path)))
      return true;
  }
  else
  #endif
  {
    IF_USE_MAIN_PATH
      if (::DeleteFileW(fs2us(path)))
        return true;
    #ifdef WIN_LONG_PATH
    if (USE_SUPER_PATH)
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        return BOOLToBool(::DeleteFileW(longPath));
    }
    #endif
  }
  return false;
}

bool RemoveDirWithSubItems(const FString &path)
{
  bool needRemoveSubItems = true;
  {
    NFind::CFileInfo fi;
    if (!fi.Find(path))
      return false;
    if (!fi.IsDir())
    {
      ::SetLastError(ERROR_DIRECTORY);
      return false;
    }
    if (fi.HasReparsePoint())
      needRemoveSubItems = false;
  }

  if (needRemoveSubItems)
  {
    FString s = path;
    s += FCHAR_PATH_SEPARATOR;
    unsigned prefixSize = s.Len();
    s += FCHAR_ANY_MASK;
    NFind::CEnumerator enumerator(s);
    NFind::CFileInfo fi;
    while (enumerator.Next(fi))
    {
      s.DeleteFrom(prefixSize);
      s += fi.Name;
      if (fi.IsDir())
      {
        if (!RemoveDirWithSubItems(s))
          return false;
      }
      else if (!DeleteFileAlways(s))
        return false;
    }
  }
  
  if (!SetFileAttrib(path, 0))
    return false;
  return RemoveDir(path);
}

#ifdef UNDER_CE

bool MyGetFullPathName(CFSTR path, FString &resFullPath)
{
  resFullPath = path;
  return true;
}

#else

bool MyGetFullPathName(CFSTR path, FString &resFullPath)
{
  return GetFullPath(path, resFullPath);
}

bool SetCurrentDir(CFSTR path)
{
  // SetCurrentDirectory doesn't support \\?\ prefix
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    return BOOLToBool(::SetCurrentDirectory(fs2fas(path)));
  }
  else
  #endif
  {
    return BOOLToBool(::SetCurrentDirectoryW(fs2us(path)));
  }
}

bool GetCurrentDir(FString &path)
{
  path.Empty();
  DWORD needLength;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetCurrentDirectory(MAX_PATH + 1, s);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetCurrentDirectoryW(MAX_PATH + 1, s);
    path = us2fs(s);
  }
  return (needLength > 0 && needLength <= MAX_PATH);
}

#endif

bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName)
{
  bool res = MyGetFullPathName(path, resDirPrefix);
  if (!res)
    resDirPrefix = path;
  int pos = resDirPrefix.ReverseFind(FCHAR_PATH_SEPARATOR);
  resFileName = resDirPrefix.Ptr(pos + 1);
  resDirPrefix.DeleteFrom(pos + 1);
  return res;
}

bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix)
{
  FString resFileName;
  return GetFullPathAndSplit(path, resDirPrefix, resFileName);
}

bool MyGetTempPath(FString &path)
{
  path.Empty();
  DWORD needLength;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    TCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetTempPath(MAX_PATH + 1, s);
    path = fas2fs(s);
  }
  else
  #endif
  {
    WCHAR s[MAX_PATH + 2];
    s[0] = 0;
    needLength = ::GetTempPathW(MAX_PATH + 1, s);;
    path = us2fs(s);
  }
  return (needLength > 0 && needLength <= MAX_PATH);
}

static bool CreateTempFile(CFSTR prefix, bool addRandom, FString &path, NIO::COutFile *outFile)
{
  UInt32 d = (GetTickCount() << 12) ^ (GetCurrentThreadId() << 14) ^ GetCurrentProcessId();
  for (unsigned i = 0; i < 100; i++)
  {
    path = prefix;
    if (addRandom)
    {
      FChar s[16];
      UInt32 value = d;
      unsigned k;
      for (k = 0; k < 8; k++)
      {
        unsigned t = value & 0xF;
        value >>= 4;
        s[k] = (char)((t < 10) ? ('0' + t) : ('A' + (t - 10)));
      }
      s[k] = '\0';
      if (outFile)
        path += FChar('.');
      path += s;
      UInt32 step = GetTickCount() + 2;
      if (step == 0)
        step = 1;
      d += step;
    }
    addRandom = true;
    if (outFile)
      path += FTEXT(".tmp");
    if (NFind::DoesFileOrDirExist(path))
    {
      SetLastError(ERROR_ALREADY_EXISTS);
      continue;
    }
    if (outFile)
    {
      if (outFile->Create(path, false))
        return true;
    }
    else
    {
      if (CreateDir(path))
        return true;
    }
    DWORD error = GetLastError();
    if (error != ERROR_FILE_EXISTS &&
        error != ERROR_ALREADY_EXISTS)
      break;
  }
  path.Empty();
  return false;
}

bool CTempFile::Create(CFSTR prefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  if (!CreateTempFile(prefix, false, _path, outFile))
    return false;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile)
{
  if (!Remove())
    return false;
  FString tempPath;
  if (!MyGetTempPath(tempPath))
    return false;
  if (!CreateTempFile(tempPath + namePrefix, true, _path, outFile))
    return false;
  _mustBeDeleted = true;
  return true;
}

bool CTempFile::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !DeleteFileAlways(_path);
  return !_mustBeDeleted;
}

bool CTempFile::MoveTo(CFSTR name, bool deleteDestBefore)
{
  if (deleteDestBefore)
    if (NFind::DoesFileExist(name))
      if (!DeleteFileAlways(name))
        return false;
  DisableDeleting();
  return MyMoveFile(_path, name);
}

bool CTempDir::Create(CFSTR prefix)
{
  if (!Remove())
    return false;
  FString tempPath;
  if (!MyGetTempPath(tempPath))
    return false;
  if (!CreateTempFile(tempPath + prefix, true, _path, NULL))
    return false;
  _mustBeDeleted = true;
  return true;
}

bool CTempDir::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !RemoveDirWithSubItems(_path);
  return !_mustBeDeleted;
}

}}}
