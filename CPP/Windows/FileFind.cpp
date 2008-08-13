// Windows/FileFind.cpp

#include "StdAfx.h"

#include "FileFind.h"
#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NFile {

#if defined(WIN_LONG_PATH) && defined(_UNICODE)
#define WIN_LONG_PATH2
#endif

bool GetLongPath(LPCWSTR fileName, UString &res);

namespace NFind {

static const TCHAR kDot = TEXT('.');

bool CFileInfo::IsDots() const
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != kDot)
    return false;
  return Name.Length() == 1 || (Name[1] == kDot && Name.Length() == 2);
}

#ifndef _UNICODE
bool CFileInfoW::IsDots() const
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != kDot)
    return false;
  return Name.Length() == 1 || (Name[1] == kDot && Name.Length() == 2);
}
#endif

static void ConvertWIN32_FIND_DATA_To_FileInfo(const WIN32_FIND_DATA &fd, CFileInfo &fi)
{
  fi.Attrib = fd.dwFileAttributes;
  fi.CTime = fd.ftCreationTime;
  fi.ATime = fd.ftLastAccessTime;
  fi.MTime = fd.ftLastWriteTime;
  fi.Size  = (((UInt64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;
  fi.Name = fd.cFileName;
  #ifndef _WIN32_WCE
  fi.ReparseTag = fd.dwReserved0;
  #else
  fi.ObjectID = fd.dwOID;
  #endif
}

#ifndef _UNICODE

static inline UINT GetCurrentCodePage() { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

static void ConvertWIN32_FIND_DATA_To_FileInfo(const WIN32_FIND_DATAW &fd, CFileInfoW &fi)
{
  fi.Attrib = fd.dwFileAttributes;
  fi.CTime = fd.ftCreationTime;
  fi.ATime = fd.ftLastAccessTime;
  fi.MTime = fd.ftLastWriteTime;
  fi.Size  = (((UInt64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;
  fi.Name = fd.cFileName;
  #ifndef _WIN32_WCE
  fi.ReparseTag = fd.dwReserved0;
  #else
  fi.ObjectID = fd.dwOID;
  #endif
}

static void ConvertWIN32_FIND_DATA_To_FileInfo(const WIN32_FIND_DATA &fd, CFileInfoW &fi)
{
  fi.Attrib = fd.dwFileAttributes;
  fi.CTime = fd.ftCreationTime;
  fi.ATime = fd.ftLastAccessTime;
  fi.MTime = fd.ftLastWriteTime;
  fi.Size  = (((UInt64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;
  fi.Name = GetUnicodeString(fd.cFileName, GetCurrentCodePage());
  #ifndef _WIN32_WCE
  fi.ReparseTag = fd.dwReserved0;
  #else
  fi.ObjectID = fd.dwOID;
  #endif
}
#endif
  
////////////////////////////////
// CFindFile

bool CFindFile::Close()
{
  if (_handle == INVALID_HANDLE_VALUE)
    return true;
  if (!::FindClose(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}

          
bool CFindFile::FindFirst(LPCTSTR wildcard, CFileInfo &fileInfo)
{
  if (!Close())
    return false;
  WIN32_FIND_DATA fd;
  _handle = ::FindFirstFile(wildcard, &fd);
  #ifdef WIN_LONG_PATH2
  if (_handle == INVALID_HANDLE_VALUE)
  {
    UString longPath;
    if (GetLongPath(wildcard, longPath))
      _handle = ::FindFirstFileW(longPath, &fd);
  }
  #endif
  if (_handle == INVALID_HANDLE_VALUE)
    return false;
  ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  return true;
}

#ifndef _UNICODE
bool CFindFile::FindFirst(LPCWSTR wildcard, CFileInfoW &fileInfo)
{
  if (!Close())
    return false;
  if (g_IsNT)
  {
    WIN32_FIND_DATAW fd;
    _handle = ::FindFirstFileW(wildcard, &fd);
    #ifdef WIN_LONG_PATH
    if (_handle == INVALID_HANDLE_VALUE)
    {
      UString longPath;
      if (GetLongPath(wildcard, longPath))
        _handle = ::FindFirstFileW(longPath, &fd);
    }
    #endif
    if (_handle != INVALID_HANDLE_VALUE)
      ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  }
  else
  {
    WIN32_FIND_DATAA fd;
    _handle = ::FindFirstFileA(UnicodeStringToMultiByte(wildcard,
        GetCurrentCodePage()), &fd);
    if (_handle != INVALID_HANDLE_VALUE)
      ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  }
  return (_handle != INVALID_HANDLE_VALUE);
}
#endif

bool CFindFile::FindNext(CFileInfo &fileInfo)
{
  WIN32_FIND_DATA fd;
  bool result = BOOLToBool(::FindNextFile(_handle, &fd));
  if (result)
    ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  return result;
}

#ifndef _UNICODE
bool CFindFile::FindNext(CFileInfoW &fileInfo)
{
  if (g_IsNT)
  {
    WIN32_FIND_DATAW fd;
    if (!::FindNextFileW(_handle, &fd))
      return false;
    ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  }
  else
  {
    WIN32_FIND_DATAA fd;
    if (!::FindNextFileA(_handle, &fd))
      return false;
    ConvertWIN32_FIND_DATA_To_FileInfo(fd, fileInfo);
  }
  return true;
}
#endif

bool FindFile(LPCTSTR wildcard, CFileInfo &fileInfo)
{
  CFindFile finder;
  return finder.FindFirst(wildcard, fileInfo);
}

#ifndef _UNICODE
bool FindFile(LPCWSTR wildcard, CFileInfoW &fileInfo)
{
  CFindFile finder;
  return finder.FindFirst(wildcard, fileInfo);
}
#endif

bool DoesFileExist(LPCTSTR name)
{
  CFileInfo fileInfo;
  return FindFile(name, fileInfo);
}

#ifndef _UNICODE
bool DoesFileExist(LPCWSTR name)
{
  CFileInfoW fileInfo;
  return FindFile(name, fileInfo);
}
#endif

/////////////////////////////////////
// CEnumerator

bool CEnumerator::NextAny(CFileInfo &fileInfo)
{
  if (_findFile.IsHandleAllocated())
    return _findFile.FindNext(fileInfo);
  else
    return _findFile.FindFirst(_wildcard, fileInfo);
}

bool CEnumerator::Next(CFileInfo &fileInfo)
{
  for (;;)
  {
    if (!NextAny(fileInfo))
      return false;
    if (!fileInfo.IsDots())
      return true;
  }
}

bool CEnumerator::Next(CFileInfo &fileInfo, bool &found)
{
  if (Next(fileInfo))
  {
    found = true;
    return true;
  }
  found = false;
  return (::GetLastError() == ERROR_NO_MORE_FILES);
}

#ifndef _UNICODE
bool CEnumeratorW::NextAny(CFileInfoW &fileInfo)
{
  if (_findFile.IsHandleAllocated())
    return _findFile.FindNext(fileInfo);
  else
    return _findFile.FindFirst(_wildcard, fileInfo);
}

bool CEnumeratorW::Next(CFileInfoW &fileInfo)
{
  for (;;)
  {
    if (!NextAny(fileInfo))
      return false;
    if (!fileInfo.IsDots())
      return true;
  }
}

bool CEnumeratorW::Next(CFileInfoW &fileInfo, bool &found)
{
  if (Next(fileInfo))
  {
    found = true;
    return true;
  }
  found = false;
  return (::GetLastError() == ERROR_NO_MORE_FILES);
}

#endif

////////////////////////////////
// CFindChangeNotification
// FindFirstChangeNotification can return 0. MSDN doesn't tell about it.

bool CFindChangeNotification::Close()
{
  if (!IsHandleAllocated())
    return true;
  if (!::FindCloseChangeNotification(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}
           
HANDLE CFindChangeNotification::FindFirst(LPCTSTR pathName, bool watchSubtree, DWORD notifyFilter)
{
  _handle = ::FindFirstChangeNotification(pathName, BoolToBOOL(watchSubtree), notifyFilter);
  #ifdef WIN_LONG_PATH2
  if (!IsHandleAllocated())
  {
    UString longPath;
    if (GetLongPath(pathName, longPath))
      _handle = ::FindFirstChangeNotificationW(longPath, BoolToBOOL(watchSubtree), notifyFilter);
  }
  #endif
  return _handle;
}

#ifndef _UNICODE
HANDLE CFindChangeNotification::FindFirst(LPCWSTR pathName, bool watchSubtree, DWORD notifyFilter)
{
  if (!g_IsNT)
    return FindFirst(UnicodeStringToMultiByte(pathName, GetCurrentCodePage()), watchSubtree, notifyFilter);
  _handle = ::FindFirstChangeNotificationW(pathName, BoolToBOOL(watchSubtree), notifyFilter);
  #ifdef WIN_LONG_PATH
  if (!IsHandleAllocated())
  {
    UString longPath;
    if (GetLongPath(pathName, longPath))
      _handle = ::FindFirstChangeNotificationW(longPath, BoolToBOOL(watchSubtree), notifyFilter);
  }
  #endif
  return _handle;
}
#endif

#ifndef _WIN32_WCE
bool MyGetLogicalDriveStrings(CSysStringVector &driveStrings)
{
  driveStrings.Clear();
  UINT32 size = GetLogicalDriveStrings(0, NULL);
  if (size == 0)
    return false;
  CSysString buffer;
  UINT32 newSize = GetLogicalDriveStrings(size, buffer.GetBuffer(size));
  if (newSize == 0)
    return false;
  if (newSize > size)
    return false;
  CSysString string;
  for(UINT32 i = 0; i < newSize; i++)
  {
    TCHAR c = buffer[i];
    if (c == TEXT('\0'))
    {
      driveStrings.Add(string);
      string.Empty();
    }
    else
      string += c;
  }
  if (!string.IsEmpty())
    return false;
  return true;
}

#ifndef _UNICODE
bool MyGetLogicalDriveStrings(UStringVector &driveStrings)
{
  driveStrings.Clear();
  if (g_IsNT)
  {
    UINT32 size = GetLogicalDriveStringsW(0, NULL);
    if (size == 0)
      return false;
    UString buffer;
    UINT32 newSize = GetLogicalDriveStringsW(size, buffer.GetBuffer(size));
    if (newSize == 0)
      return false;
    if (newSize > size)
      return false;
    UString string;
    for(UINT32 i = 0; i < newSize; i++)
    {
      WCHAR c = buffer[i];
      if (c == L'\0')
      {
        driveStrings.Add(string);
        string.Empty();
      }
      else
        string += c;
    }
    return string.IsEmpty();
  }
  CSysStringVector driveStringsA;
  bool res = MyGetLogicalDriveStrings(driveStringsA);
  for (int i = 0; i < driveStringsA.Size(); i++)
    driveStrings.Add(GetUnicodeString(driveStringsA[i]));
  return res;
}
#endif

#endif

}}}
