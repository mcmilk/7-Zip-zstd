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
    const WIN32_FIND_DATA &aFindData,
    CFileInfo &aFileInfo)
{
  aFileInfo.Attributes = aFindData.dwFileAttributes; 
  aFileInfo.CreationTime = aFindData.ftCreationTime;  
  aFileInfo.LastAccessTime = aFindData.ftLastAccessTime; 
  aFileInfo.LastWriteTime = aFindData.ftLastWriteTime;
  aFileInfo.Size  = (((UINT64)aFindData.nFileSizeHigh) << 32) + 
      aFindData.nFileSizeLow; 
  aFileInfo.Name = aFindData.cFileName;
  #ifndef _WIN32_WCE
  aFileInfo.ReparseTag = aFindData.dwReserved0;
  #else
  aFileInfo.ObjectID = aFindData.dwOID;
  #endif
}
  
////////////////////////////////
// CFindFile

bool CFindFile::Close()
{
  if(!m_HandleAllocated)
    return true;
  bool aResult = BOOLToBool(::FindClose(m_Handle));
  m_HandleAllocated = !aResult;
  return aResult;
}
           
bool CFindFile::FindFirst(LPCTSTR aWildcard, CFileInfo &aFileInfo)
{
  Close();
  WIN32_FIND_DATA aFindData;
  m_Handle = ::FindFirstFile(aWildcard, &aFindData);
  m_HandleAllocated = (m_Handle != INVALID_HANDLE_VALUE);
  if (m_HandleAllocated)
    ConvertWIN32_FIND_DATA_To_FileInfo(aFindData, aFileInfo);
  return m_HandleAllocated;
}

bool CFindFile::FindNext(CFileInfo &aFileInfo)
{
  WIN32_FIND_DATA aFindData;
  bool aResult = BOOLToBool(::FindNextFile(m_Handle, &aFindData));
  if (aResult)
    ConvertWIN32_FIND_DATA_To_FileInfo(aFindData, aFileInfo);
  return aResult;
}

bool FindFile(LPCTSTR aWildcard, CFileInfo &aFileInfo)
{
  CFindFile aFinder;
  return aFinder.FindFirst(aWildcard, aFileInfo);
}

bool DoesFileExist(LPCTSTR aName)
{
  CFileInfo aFileInfo;
  return FindFile(aName, aFileInfo);
}

/////////////////////////////////////
// CEnumerator

bool CEnumerator::NextAny(CFileInfo &aFileInfo)
{
  if(m_FindFile.IsHandleAllocated())
    return m_FindFile.FindNext(aFileInfo);
  else
    return m_FindFile.FindFirst(m_Wildcard, aFileInfo);
}

bool CEnumerator::Next(CFileInfo &aFileInfo)
{
  while(true)
  {
    if(!NextAny(aFileInfo))
      return false;
    if(!aFileInfo.IsDots())
      return true;
  }
}


////////////////////////////////
// CFindChangeNotification

bool CFindChangeNotification::Close()
{
  if(m_Handle == INVALID_HANDLE_VALUE)
    return true;
  bool aResult = BOOLToBool(::FindCloseChangeNotification(m_Handle));
  if (aResult)
    m_Handle = INVALID_HANDLE_VALUE;
  return aResult;
}
           
HANDLE CFindChangeNotification::FindFirst(LPCTSTR aPathName, bool aWatchSubtree, 
    DWORD aNotifyFilter)
{
  m_Handle = ::FindFirstChangeNotification(aPathName, 
      BoolToBOOL(aWatchSubtree), aNotifyFilter);
  return m_Handle;
}

#ifndef _WIN32_WCE
bool MyGetLogicalDriveStrings(CSysStringVector &aDriveStrings)
{
  aDriveStrings.Clear();
  UINT32 aSize = GetLogicalDriveStrings( 0, NULL); 
  if(aSize == 0)
    return false;
  CSysString aBuffer;
  UINT32 aNewSize = GetLogicalDriveStrings(aSize, aBuffer.GetBuffer(aSize)); 
  if(aNewSize == 0)
    return false;
  if(aNewSize > aSize)
    return false;
  CSysString aString;
  for(UINT32 i = 0; i < aNewSize; i++)
  {
    TCHAR aChar = aBuffer[i];
    if(aChar == TEXT('\0'))
    {
      aDriveStrings.Add(aString);
      aString.Empty();
    }
    else
      aString += aChar;
  }
  if(!aString.IsEmpty())
    return false;
  return true;
}
#endif

}}}
