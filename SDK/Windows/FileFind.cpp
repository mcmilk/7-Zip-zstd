// Windows/FileFind.cpp

#include "StdAfx.h"

#include "Windows/FileFind.h"

namespace NWindows {
namespace NFile {
namespace NFind {

static const TCHAR kDot = _T('.');

bool CFileInfo::IsDots() const
{ 
  if (!IsDirectory() || Name.IsEmpty())
    return false;
  if (Name[0] != kDot)
    return false;
  return Name.Length() == 1 || (Name[1] == kDot && Name.Length() == 2);
}

static void ConvertWIN32_FIND_DATA_To_FileInfo(
    const WIN32_FIND_DATA &findData,
    CFileInfo &fileInfo)
{
  fileInfo.Attributes = findData.dwFileAttributes; 
  fileInfo.CreationTime = findData.ftCreationTime;  
  fileInfo.LastAccessTime = findData.ftLastAccessTime; 
  fileInfo.LastWriteTime = findData.ftLastWriteTime;
  fileInfo.Size  = (((UINT64)findData.nFileSizeHigh) << 32) + 
      findData.nFileSizeLow; 
  fileInfo.Name = findData.cFileName;
  #ifndef _WIN32_WCE
  fileInfo.ReparseTag = findData.dwReserved0;
  #else
  fileInfo.ObjectID = findData.dwOID;
  #endif
}
  
////////////////////////////////
// CFindFile

bool CFindFile::Close()
{
  if(!_handleAllocated)
    return true;
  bool result = BOOLToBool(::FindClose(_handle));
  _handleAllocated = !result;
  return result;
}
           
bool CFindFile::FindFirst(LPCTSTR wildcard, CFileInfo &fileInfo)
{
  Close();
  WIN32_FIND_DATA findData;
  _handle = ::FindFirstFile(wildcard, &findData);
  _handleAllocated = (_handle != INVALID_HANDLE_VALUE);
  if (_handleAllocated)
    ConvertWIN32_FIND_DATA_To_FileInfo(findData, fileInfo);
  return _handleAllocated;
}

bool CFindFile::FindNext(CFileInfo &fileInfo)
{
  WIN32_FIND_DATA findData;
  bool result = BOOLToBool(::FindNextFile(_handle, &findData));
  if (result)
    ConvertWIN32_FIND_DATA_To_FileInfo(findData, fileInfo);
  return result;
}

bool FindFile(LPCTSTR wildcard, CFileInfo &fileInfo)
{
  CFindFile finder;
  return finder.FindFirst(wildcard, fileInfo);
}

bool DoesFileExist(LPCTSTR name)
{
  CFileInfo fileInfo;
  return FindFile(name, fileInfo);
}

/////////////////////////////////////
// CEnumerator

bool CEnumerator::NextAny(CFileInfo &fileInfo)
{
  if(_findFile.IsHandleAllocated())
    return _findFile.FindNext(fileInfo);
  else
    return _findFile.FindFirst(_wildcard, fileInfo);
}

bool CEnumerator::Next(CFileInfo &fileInfo)
{
  while(true)
  {
    if(!NextAny(fileInfo))
      return false;
    if(!fileInfo.IsDots())
      return true;
  }
}


////////////////////////////////
// CFindChangeNotification

bool CFindChangeNotification::Close()
{
  if(_handle == INVALID_HANDLE_VALUE)
    return true;
  bool result = BOOLToBool(::FindCloseChangeNotification(_handle));
  if (result)
    _handle = INVALID_HANDLE_VALUE;
  return result;
}
           
HANDLE CFindChangeNotification::FindFirst(LPCTSTR pathName, bool watchSubtree, 
    DWORD notifyFilter)
{
  _handle = ::FindFirstChangeNotification(pathName, 
      BoolToBOOL(watchSubtree), notifyFilter);
  return _handle;
}

#ifndef _WIN32_WCE
bool MyGetLogicalDriveStrings(CSysStringVector &driveStrings)
{
  driveStrings.Clear();
  UINT32 size = GetLogicalDriveStrings( 0, NULL); 
  if(size == 0)
    return false;
  CSysString buffer;
  UINT32 newSize = GetLogicalDriveStrings(size, buffer.GetBuffer(size)); 
  if(newSize == 0)
    return false;
  if(newSize > size)
    return false;
  CSysString string;
  for(UINT32 i = 0; i < newSize; i++)
  {
    TCHAR c = buffer[i];
    if(c == TEXT('\0'))
    {
      driveStrings.Add(string);
      string.Empty();
    }
    else
      string += c;
  }
  if(!string.IsEmpty())
    return false;
  return true;
}
#endif

}}}
