// Windows/FileDir.cpp

#include "StdAfx.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"

#include "Windows/Defs.h"
#include "Windows/System.h"

namespace NWindows {
namespace NFile {
namespace NDirectory {

bool MyCreateDirectory(LPCTSTR aPathName)
{  
  return BOOLToBool(::CreateDirectory(aPathName, NULL)); 
}

/*
bool CreateComplexDirectory(LPCTSTR aPathName)
{
  NName::CParsedPath aPath;
  aPath.ParsePath(aPathName);
  CSysString aFullPath = aPath.Prefix;
  DWORD anErrorCode = ERROR_SUCCESS;
  for(int i = 0; i < aPath.PathParts.Size(); i++)
  {
    const CSysString &aString = aPath.PathParts[i];
    if(aString.IsEmpty())
    {
      if(i != aPath.PathParts.Size() - 1)
        return false;
      return true;
    }
    aFullPath += aPath.PathParts[i];
    if(!MyCreateDirectory(aFullPath))
    {
      DWORD anErrorCode = GetLastError();
      if(anErrorCode != ERROR_ALREADY_EXISTS)
        return false;
    }
    aFullPath += NName::kDirDelimiter;
  }
  return true;
}
*/

bool CreateComplexDirectory(LPCTSTR _aPathName)
{
  CSysString aPathName = _aPathName;
  int aPos = aPathName.ReverseFind(TEXT('\\'));
  if (aPos > 0 && aPos == aPathName.Length() - 1)
  {
    if (aPathName.Length() == 3 && aPathName[1] == ':')
      return true; // Disk folder;
    aPathName.Delete(aPos);
  }
  CSysString aPathName2 = aPathName;
  aPos = aPathName.Length();
  while(true)
  {
    if(MyCreateDirectory(aPathName))
      break;
    if(::GetLastError() == ERROR_ALREADY_EXISTS)
      break;
    aPos = aPathName.ReverseFind(TEXT('\\'));
    if (aPos < 0 || aPos == 0)
      return false;
    if (aPathName[aPos - 1] == ':')
      return false;
    aPathName = aPathName.Left(aPos);
  }
  aPathName = aPathName2;
  while(aPos < aPathName.Length())
  {
    aPos = aPathName.Find(TEXT('\\'), aPos + 1);
    if (aPos < 0)
      aPos = aPathName.Length();
    if(!MyCreateDirectory(aPathName.Left(aPos)))
      return false;
  }
  return true;
}

bool DeleteFileAlways(LPCTSTR aName)
{
  if(!BOOLToBool(::SetFileAttributes(aName, 0)))
    return false;
  return BOOLToBool(::DeleteFile(aName));
}

static bool RemoveDirectorySubItems2(const CSysString aPathPrefix,
    const NFind::CFileInfo &aFileInfo)
{
  if(aFileInfo.IsDirectory())
    return RemoveDirectoryWithSubItems(aPathPrefix + aFileInfo.Name);
  else
    return DeleteFileAlways(aPathPrefix + aFileInfo.Name);
}

bool RemoveDirectoryWithSubItems(const CSysString &aPath)
{
  NFind::CFileInfo aFileInfo;
  CSysString aPathPrefix = aPath + NName::kDirDelimiter;
  {
    NFind::CEnumerator anEnumerator(aPathPrefix + TCHAR(NName::kAnyStringWildcard));
    while(anEnumerator.Next(aFileInfo))
      if(!RemoveDirectorySubItems2(aPathPrefix, aFileInfo))
        return false;
  }
  return BOOLToBool(::RemoveDirectory(aPath));
}


#ifndef _WIN32_WCE
bool MyGetFullPathName(LPCTSTR aFileName, CSysString &aResultPath, 
    int &aFileNamePartStartIndex)
{
  LPTSTR aFileNamePointer;
  LPTSTR aBuffer = aResultPath.GetBuffer(MAX_PATH);
  DWORD aNeedLength = ::GetFullPathName(aFileName, MAX_PATH + 1, 
      aBuffer, &aFileNamePointer);
  aResultPath.ReleaseBuffer();
  if (aNeedLength == 0 || aNeedLength >= MAX_PATH)
    return false;
  aFileNamePartStartIndex = aFileNamePointer - aBuffer;
  return true;
}

bool MyGetFullPathName(LPCTSTR aFileName, CSysString &aResultPath)
{
  int anIndex;
  return MyGetFullPathName(aFileName, aResultPath, anIndex);
}

bool GetOnlyName(LPCTSTR aFileName, CSysString &aResultName)
{
  int anIndex;
  if (!MyGetFullPathName(aFileName, aResultName, anIndex))
    return false;
  aResultName = aResultName.Mid(anIndex);
  return true;
}

bool GetOnlyDirPrefix(LPCTSTR aFileName, CSysString &aResultName)
{
  int anIndex;
  if (!MyGetFullPathName(aFileName, aResultName, anIndex))
    return false;
  aResultName = aResultName.Left(anIndex);
  return true;
}

bool MyGetCurrentDirectory(CSysString &aResultPath)
{
  DWORD aNeedLength = ::GetCurrentDirectory(MAX_PATH + 1, 
      aResultPath.GetBuffer(MAX_PATH));
  aResultPath.ReleaseBuffer();
  return (aNeedLength > 0 && aNeedLength < MAX_PATH);
}
#endif

bool MySearchPath(LPCTSTR aPath, LPCTSTR aFileName, LPCTSTR anExtension, 
  CSysString &aResultPath, UINT32 &aFilePart)
{
  LPTSTR lpFilePart;
  DWORD aValue = SearchPath(aPath, aFileName, anExtension, 
    MAX_PATH, aResultPath.GetBuffer(MAX_PATH), &lpFilePart);
  aResultPath.ReleaseBuffer();
  if (aValue == 0 || aValue > MAX_PATH)
    return false;
  return true;
}


bool MyGetTempPath(CSysString &aResultPath)
{
  DWORD aNeedLength = ::GetTempPath(MAX_PATH + 1, 
      aResultPath.GetBuffer(MAX_PATH));
  aResultPath.ReleaseBuffer();
  return (aNeedLength > 0 && aNeedLength <= MAX_PATH);
}

UINT MyGetTempFileName(LPCTSTR aDirPath, LPCTSTR aPrefix, CSysString &aResultPath)
{
  UINT aNumber = ::GetTempFileName(aDirPath, aPrefix, 0,
      aResultPath.GetBuffer(MAX_PATH));
  aResultPath.ReleaseBuffer();
  return aNumber;
}

UINT CTempFile::Create(LPCTSTR aDirPath, LPCTSTR aPrefix, 
    CSysString &aResultPath)
{
  Remove();
  UINT aNumber = MyGetTempFileName(aDirPath, aPrefix, aResultPath);
  if(aNumber != 0)
  {
    m_FileName = aResultPath;
    m_MustBeDeleted = true;
  }
  return aNumber;
}

bool CTempFile::Create(LPCTSTR aPrefix, CSysString &aResultPath)
{
  CSysString aTempPath;
  if(!MyGetTempPath(aTempPath))
    return false;
  if (Create(aTempPath, aPrefix, aResultPath) != 0)
    return true;
  if(!NSystem::MyGetWindowsDirectory(aTempPath))
    return false;
  return (Create(aTempPath, aPrefix, aResultPath) != 0);
}

bool CTempFile::Remove()
{
  if (!m_MustBeDeleted)
    return true;
  m_MustBeDeleted = !DeleteFileAlways(m_FileName);
  return !m_MustBeDeleted;
}

bool CreateTempDirectory(LPCTSTR aPrefix, CSysString &aDirName)
{
  /*
  CSysString aPrefix = aTempPath + aPrefixChars;
  CRandom aRandom;
  aRandom.Init();
  */
  while(true)
  {
    CTempFile aTempFile;
    if (!aTempFile.Create(aPrefix, aDirName))
      return false;
    if (!::DeleteFile(aDirName))
      return false;
    /*
    UINT32 aRandomNumber = aRandom.Generate();
    TCHAR aRandomNumberString[32];
    _stprintf(aRandomNumberString, _T("%04X"), aRandomNumber);
    aDirName = aPrefix + aRandomNumberString;
    */
    if(NFile::NFind::DoesFileExist(aDirName))
      continue;
    bool aResult = NFile::NDirectory::MyCreateDirectory(aDirName);
    if (aResult)
      return true;
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }
}

bool CTempDirectory::Create(LPCTSTR aPrefix)
{ 
  Remove();
  return (m_MustBeDeleted = CreateTempDirectory(aPrefix, m_TempDir)); 
}


}}}
