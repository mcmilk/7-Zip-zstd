// Windows/FileFind.cpp

#include "StdAfx.h"

#include "FileFind.h"
#include "FileIO.h"
#include "FileName.h"
#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

#ifndef _UNICODE
extern bool g_IsNT;
#endif

using namespace NWindows;
using namespace NFile;
using namespace NName;

#if defined(_WIN32) && !defined(UNDER_CE)

EXTERN_C_BEGIN

typedef enum
{
  My_FindStreamInfoStandard,
  My_FindStreamInfoMaxInfoLevel
} MY_STREAM_INFO_LEVELS;

typedef struct
{
  LARGE_INTEGER StreamSize;
  WCHAR cStreamName[MAX_PATH + 36];
} MY_WIN32_FIND_STREAM_DATA, *MY_PWIN32_FIND_STREAM_DATA;

typedef WINBASEAPI HANDLE (WINAPI *FindFirstStreamW_Ptr)(LPCWSTR fileName, MY_STREAM_INFO_LEVELS infoLevel,
    LPVOID findStreamData, DWORD flags);

typedef WINBASEAPI BOOL (APIENTRY *FindNextStreamW_Ptr)(HANDLE findStream, LPVOID findStreamData);

EXTERN_C_END

#endif

namespace NWindows {
namespace NFile {

#ifdef SUPPORT_DEVICE_FILE
namespace NSystem
{
bool MyGetDiskFreeSpace(CFSTR rootPath, UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);
}
#endif

namespace NFind {

bool CFileInfo::IsDots() const
{
  if (!IsDir() || Name.IsEmpty())
    return false;
  if (Name[0] != FTEXT('.'))
    return false;
  return Name.Len() == 1 || (Name.Len() == 2 && Name[1] == FTEXT('.'));
}

#define WIN_FD_TO_MY_FI(fi, fd) \
  fi.Attrib = fd.dwFileAttributes; \
  fi.CTime = fd.ftCreationTime; \
  fi.ATime = fd.ftLastAccessTime; \
  fi.MTime = fd.ftLastWriteTime; \
  fi.Size = (((UInt64)fd.nFileSizeHigh) << 32) + fd.nFileSizeLow; \
  fi.IsAltStream = false; \
  fi.IsDevice = false;

  /*
  #ifdef UNDER_CE
  fi.ObjectID = fd.dwOID;
  #else
  fi.ReparseTag = fd.dwReserved0;
  #endif
  */

static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATAW &fd, CFileInfo &fi)
{
  WIN_FD_TO_MY_FI(fi, fd);
  fi.Name = us2fs(fd.cFileName);
  #if defined(_WIN32) && !defined(UNDER_CE)
  // fi.ShortName = us2fs(fd.cAlternateFileName);
  #endif
}

#ifndef _UNICODE

static void Convert_WIN32_FIND_DATA_to_FileInfo(const WIN32_FIND_DATA &fd, CFileInfo &fi)
{
  WIN_FD_TO_MY_FI(fi, fd);
  fi.Name = fas2fs(fd.cFileName);
  #if defined(_WIN32) && !defined(UNDER_CE)
  // fi.ShortName = fas2fs(fd.cAlternateFileName);
  #endif
}
#endif
  
////////////////////////////////
// CFindFile

bool CFindFileBase::Close()
{
  if (_handle == INVALID_HANDLE_VALUE)
    return true;
  if (!::FindClose(_handle))
    return false;
  _handle = INVALID_HANDLE_VALUE;
  return true;
}

bool CFindFile::FindFirst(CFSTR path, CFileInfo &fi)
{
  if (!Close())
    return false;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    WIN32_FIND_DATAA fd;
    _handle = ::FindFirstFileA(fs2fas(path), &fd);
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  else
  #endif
  {
    WIN32_FIND_DATAW fd;

    IF_USE_MAIN_PATH
      _handle = ::FindFirstFileW(fs2us(path), &fd);
    #ifdef WIN_LONG_PATH
    if (_handle == INVALID_HANDLE_VALUE && USE_SUPER_PATH)
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        _handle = ::FindFirstFileW(longPath, &fd);
    }
    #endif
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  return true;
}

bool CFindFile::FindNext(CFileInfo &fi)
{
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    WIN32_FIND_DATAA fd;
    if (!::FindNextFileA(_handle, &fd))
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  else
  #endif
  {
    WIN32_FIND_DATAW fd;
    if (!::FindNextFileW(_handle, &fd))
      return false;
    Convert_WIN32_FIND_DATA_to_FileInfo(fd, fi);
  }
  return true;
}

#if defined(_WIN32) && !defined(UNDER_CE)

////////////////////////////////
// AltStreams

static FindFirstStreamW_Ptr g_FindFirstStreamW;
static FindNextStreamW_Ptr g_FindNextStreamW;

struct CFindStreamLoader
{
  CFindStreamLoader()
  {
    g_FindFirstStreamW = (FindFirstStreamW_Ptr)::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "FindFirstStreamW");
    g_FindNextStreamW = (FindNextStreamW_Ptr)::GetProcAddress(::GetModuleHandleA("kernel32.dll"), "FindNextStreamW");
  }
} g_FindStreamLoader;

bool CStreamInfo::IsMainStream() const
{
  return Name == L"::$DATA";
};

UString CStreamInfo::GetReducedName() const
{
  UString s = Name;
  if (s.Len() >= 6)
    if (wcscmp(s.RightPtr(6), L":$DATA") == 0)
      s.DeleteFrom(s.Len() - 6);
  return s;
}

static void Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(const MY_WIN32_FIND_STREAM_DATA &sd, CStreamInfo &si)
{
  si.Size = sd.StreamSize.QuadPart;
  si.Name = sd.cStreamName;
}

bool CFindStream::FindFirst(CFSTR path, CStreamInfo &si)
{
  if (!Close())
    return false;
  if (!g_FindFirstStreamW)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
  {
    MY_WIN32_FIND_STREAM_DATA sd;
    IF_USE_MAIN_PATH
      _handle = g_FindFirstStreamW(fs2us(path), My_FindStreamInfoStandard, &sd, 0);
    if (_handle == INVALID_HANDLE_VALUE)
    {
      if (::GetLastError() == ERROR_HANDLE_EOF)
        return false;
      // long name can be tricky for path like ".\dirName".
      #ifdef WIN_LONG_PATH
      if (USE_SUPER_PATH)
      {
        UString longPath;
        if (GetSuperPath(path, longPath, USE_MAIN_PATH))
          _handle = g_FindFirstStreamW(longPath, My_FindStreamInfoStandard, &sd, 0);
      }
      #endif
    }
    if (_handle == INVALID_HANDLE_VALUE)
      return false;
    Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
  }
  return true;
}

bool CFindStream::FindNext(CStreamInfo &si)
{
  if (!g_FindNextStreamW)
  {
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return false;
  }
  {
    MY_WIN32_FIND_STREAM_DATA sd;
    if (!g_FindNextStreamW(_handle, &sd))
      return false;
    Convert_WIN32_FIND_STREAM_DATA_to_StreamInfo(sd, si);
  }
  return true;
}

bool CStreamEnumerator::Next(CStreamInfo &si, bool &found)
{
  bool res;
  if (_find.IsHandleAllocated())
    res = _find.FindNext(si);
  else
    res = _find.FindFirst(_filePath, si);
  if (res)
  {
    found = true;
    return true;
  }
  found = false;
  return (::GetLastError() == ERROR_HANDLE_EOF);
}

#endif


#define MY_CLEAR_FILETIME(ft) ft.dwLowDateTime = ft.dwHighDateTime = 0;

void CFileInfoBase::Clear()
{
  Size = 0;
  MY_CLEAR_FILETIME(CTime);
  MY_CLEAR_FILETIME(ATime);
  MY_CLEAR_FILETIME(MTime);
  Attrib = 0;
  IsAltStream = false;
  IsDevice = false;
}

#if defined(_WIN32) && !defined(UNDER_CE)

static int FindAltStreamColon(CFSTR path)
{
  for (int i = 0;; i++)
  {
    FChar c = path[i];
    if (c == 0)
      return -1;
    if (c == ':')
    {
      if (path[i + 1] == '\\')
        if (i == 1 || (i > 1 && path[i - 2] == '\\'))
        {
          wchar_t c0 = path[i - 1];
          if (c0 >= 'a' && c0 <= 'z' ||
              c0 >= 'A' && c0 <= 'Z')
            continue;
        }
      return i;
    }
  }
}

#endif

bool CFileInfo::Find(CFSTR path)
{
  #ifdef SUPPORT_DEVICE_FILE
  if (IsDevicePath(path))
  {
    Clear();
    Name = path + 4;

    IsDevice = true;
    if (/* path[0] == '\\' && path[1] == '\\' && path[2] == '.' && path[3] == '\\' && */
        path[5] == ':' && path[6] == 0)
    {
      FChar drive[4] = { path[4], ':', '\\', 0 };
      UInt64 clusterSize, totalSize, freeSize;
      if (NSystem::MyGetDiskFreeSpace(drive, clusterSize, totalSize, freeSize))
      {
        Size = totalSize;
        return true;
      }
    }

    NIO::CInFile inFile;
    // ::OutputDebugStringW(path);
    if (!inFile.Open(path))
      return false;
    // ::OutputDebugStringW(L"---");
    if (inFile.SizeDefined)
      Size = inFile.Size;
    return true;
  }
  #endif

  #if defined(_WIN32) && !defined(UNDER_CE)

  int colonPos = FindAltStreamColon(path);
  if (colonPos >= 0)
  {
    UString streamName = fs2us(path + (unsigned)colonPos);
    FString filePath = path;
    filePath.DeleteFrom(colonPos);
    streamName += L":$DATA"; // change it!!!!
    if (Find(filePath))
    {
      // if (IsDir())
        Attrib &= ~FILE_ATTRIBUTE_DIRECTORY;
      Size = 0;
      CStreamEnumerator enumerator(filePath);
      for (;;)
      {
        CStreamInfo si;
        bool found;
        if (!enumerator.Next(si, found))
          return false;
        if (!found)
        {
          ::SetLastError(ERROR_FILE_NOT_FOUND);
          return false;
        }
        if (si.Name.IsEqualToNoCase(streamName))
        {
          Name += us2fs(si.Name);
          Name.DeleteFrom(Name.Len() - 6);
          Size = si.Size;
          IsAltStream = true;
          return true;
        }
      }
    }
  }
  
  #endif

  CFindFile finder;
  if (finder.FindFirst(path, *this))
    return true;
  #ifdef _WIN32
  {
    DWORD lastError = GetLastError();
    if (lastError == ERROR_BAD_NETPATH ||
        lastError == ERROR_FILE_NOT_FOUND ||
        lastError == ERROR_INVALID_NAME // for "\\SERVER\shared" paths that are translated to "\\?\UNC\SERVER\shared"
        )
    {
      unsigned len = MyStringLen(path);
      if (len > 2 && path[0] == '\\' && path[1] == '\\')
      {
        int startPos = 2;
        if (len > kSuperUncPathPrefixSize && IsSuperUncPath(path))
          startPos = kSuperUncPathPrefixSize;
        int pos = FindCharPosInString(path + startPos, FTEXT('\\'));
        if (pos >= 0)
        {
          pos += startPos + 1;
          len -= pos;
          int pos2 = FindCharPosInString(path + pos, FTEXT('\\'));
          if (pos2 < 0 || pos2 == (int)len - 1)
          {
            FString s = path;
            if (pos2 < 0)
            {
              pos2 = len;
              s += FTEXT('\\');
            }
            s += FCHAR_ANY_MASK;
            if (finder.FindFirst(s, *this))
              if (Name == FTEXT("."))
              {
                Name.SetFrom(s.Ptr(pos), pos2);
                return true;
              }
            ::SetLastError(lastError);
          }
        }
      }
    }
  }
  #endif
  return false;
}

bool DoesFileExist(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name) && !fi.IsDir();
}

bool DoesDirExist(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name) && fi.IsDir();
}
bool DoesFileOrDirExist(CFSTR name)
{
  CFileInfo fi;
  return fi.Find(name);
}

bool CEnumerator::NextAny(CFileInfo &fi)
{
  if (_findFile.IsHandleAllocated())
    return _findFile.FindNext(fi);
  else
    return _findFile.FindFirst(_wildcard, fi);
}

bool CEnumerator::Next(CFileInfo &fi)
{
  for (;;)
  {
    if (!NextAny(fi))
      return false;
    if (!fi.IsDots())
      return true;
  }
}

bool CEnumerator::Next(CFileInfo &fi, bool &found)
{
  if (Next(fi))
  {
    found = true;
    return true;
  }
  found = false;
  return (::GetLastError() == ERROR_NO_MORE_FILES);
}

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
           
HANDLE CFindChangeNotification::FindFirst(CFSTR path, bool watchSubtree, DWORD notifyFilter)
{
  #ifndef _UNICODE
  if (!g_IsNT)
    _handle = ::FindFirstChangeNotification(fs2fas(path), BoolToBOOL(watchSubtree), notifyFilter);
  else
  #endif
  {
    IF_USE_MAIN_PATH
    _handle = ::FindFirstChangeNotificationW(fs2us(path), BoolToBOOL(watchSubtree), notifyFilter);
    #ifdef WIN_LONG_PATH
    if (!IsHandleAllocated())
    {
      UString longPath;
      if (GetSuperPath(path, longPath, USE_MAIN_PATH))
        _handle = ::FindFirstChangeNotificationW(longPath, BoolToBOOL(watchSubtree), notifyFilter);
    }
    #endif
  }
  return _handle;
}

#ifndef UNDER_CE

bool MyGetLogicalDriveStrings(CObjectVector<FString> &driveStrings)
{
  driveStrings.Clear();
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    driveStrings.Clear();
    UINT32 size = GetLogicalDriveStrings(0, NULL);
    if (size == 0)
      return false;
    AString buf;
    UINT32 newSize = GetLogicalDriveStrings(size, buf.GetBuffer(size));
    if (newSize == 0 || newSize > size)
      return false;
    AString s;
    for (UINT32 i = 0; i < newSize; i++)
    {
      char c = buf[i];
      if (c == '\0')
      {
        driveStrings.Add(fas2fs(s));
        s.Empty();
      }
      else
        s += c;
    }
    return s.IsEmpty();
  }
  else
  #endif
  {
    UINT32 size = GetLogicalDriveStringsW(0, NULL);
    if (size == 0)
      return false;
    UString buf;
    UINT32 newSize = GetLogicalDriveStringsW(size, buf.GetBuffer(size));
    if (newSize == 0 || newSize > size)
      return false;
    UString s;
    for (UINT32 i = 0; i < newSize; i++)
    {
      WCHAR c = buf[i];
      if (c == L'\0')
      {
        driveStrings.Add(us2fs(s));
        s.Empty();
      }
      else
        s += c;
    }
    return s.IsEmpty();
  }
}

#endif

}}}
