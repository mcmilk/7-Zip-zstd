// Windows/FileDir.cpp

#include "StdAfx.h"

#include "FileDir.h"
#include "FileName.h"
#include "FileFind.h"
#include "Defs.h"
#include "System.h"

namespace NWindows {
namespace NFile {
namespace NDirectory {

bool MyCreateDirectory(LPCTSTR pathName)
{  
  return BOOLToBool(::CreateDirectory(pathName, NULL)); 
}

/*
bool CreateComplexDirectory(LPCTSTR pathName)
{
  NName::CParsedPath path;
  path.ParsePath(pathName);
  CSysString fullPath = path.Prefix;
  DWORD errorCode = ERROR_SUCCESS;
  for(int i = 0; i < path.PathParts.Size(); i++)
  {
    const CSysString &string = path.PathParts[i];
    if(string.IsEmpty())
    {
      if(i != path.PathParts.Size() - 1)
        return false;
      return true;
    }
    fullPath += path.PathParts[i];
    if(!MyCreateDirectory(fullPath))
    {
      DWORD errorCode = GetLastError();
      if(errorCode != ERROR_ALREADY_EXISTS)
        return false;
    }
    fullPath += NName::kDirDelimiter;
  }
  return true;
}
*/

bool CreateComplexDirectory(LPCTSTR _aPathName)
{
  CSysString pathName = _aPathName;
  int pos = pathName.ReverseFind(TEXT('\\'));
  if (pos > 0 && pos == pathName.Length() - 1)
  {
    if (pathName.Length() == 3 && pathName[1] == ':')
      return true; // Disk folder;
    pathName.Delete(pos);
  }
  CSysString pathName2 = pathName;
  pos = pathName.Length();
  while(true)
  {
    if(MyCreateDirectory(pathName))
      break;
    DWORD lastError = ::GetLastError();
    if(lastError == ERROR_ALREADY_EXISTS)
    {
      NFind::CFileInfo fileInfo;
      if (!NFind::FindFile(pathName, fileInfo)) // For network folders
        return true;
      if (!fileInfo.IsDirectory())
        return false;
      break;
    }
    pos = pathName.ReverseFind(TEXT('\\'));
    if (pos < 0 || pos == 0)
      return false;
    if (pathName[pos - 1] == ':')
      return false;
    pathName = pathName.Left(pos);
  }
  pathName = pathName2;
  while(pos < pathName.Length())
  {
    pos = pathName.Find(TEXT('\\'), pos + 1);
    if (pos < 0)
      pos = pathName.Length();
    if(!MyCreateDirectory(pathName.Left(pos)))
      return false;
  }
  return true;
}

bool DeleteFileAlways(LPCTSTR name)
{
  if(!BOOLToBool(::SetFileAttributes(name, 0)))
    return false;
  return BOOLToBool(::DeleteFile(name));
}

static bool RemoveDirectorySubItems2(const CSysString pathPrefix,
    const NFind::CFileInfo &fileInfo)
{
  if(fileInfo.IsDirectory())
    return RemoveDirectoryWithSubItems(pathPrefix + fileInfo.Name);
  else
    return DeleteFileAlways(pathPrefix + fileInfo.Name);
}

bool RemoveDirectoryWithSubItems(const CSysString &path)
{
  NFind::CFileInfo fileInfo;
  CSysString pathPrefix = path + NName::kDirDelimiter;
  {
    NFind::CEnumerator enumerator(pathPrefix + TCHAR(NName::kAnyStringWildcard));
    while(enumerator.Next(fileInfo))
      if(!RemoveDirectorySubItems2(pathPrefix, fileInfo))
        return false;
  }
  if(!BOOLToBool(::SetFileAttributes(path, 0)))
    return false;
  return BOOLToBool(::RemoveDirectory(path));
}


#ifndef _WIN32_WCE

bool MyGetShortPathName(LPCTSTR longPath, CSysString &shortPath)
{
  DWORD needLength = ::GetShortPathName(longPath, 
      shortPath.GetBuffer(MAX_PATH + 1), MAX_PATH + 1);
  shortPath.ReleaseBuffer();
  if (needLength == 0 || needLength >= MAX_PATH)
    return false;
  return true;
}

bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath, 
    int &fileNamePartStartIndex)
{
  LPTSTR fileNamePointer = 0;
  LPTSTR buffer = resultPath.GetBuffer(MAX_PATH);
  DWORD needLength = ::GetFullPathName(fileName, MAX_PATH + 1, 
      buffer, &fileNamePointer);
  resultPath.ReleaseBuffer();
  if (needLength == 0 || needLength >= MAX_PATH)
    return false;
  if (fileNamePointer == 0)
    fileNamePartStartIndex = lstrlen(fileName);
  else
    fileNamePartStartIndex = fileNamePointer - buffer;
  return true;
}

bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath)
{
  int index;
  return MyGetFullPathName(fileName, resultPath, index);
}

bool GetOnlyName(LPCTSTR fileName, CSysString &resultName)
{
  int index;
  if (!MyGetFullPathName(fileName, resultName, index))
    return false;
  resultName = resultName.Mid(index);
  return true;
}

bool GetOnlyDirPrefix(LPCTSTR fileName, CSysString &resultName)
{
  int index;
  if (!MyGetFullPathName(fileName, resultName, index))
    return false;
  resultName = resultName.Left(index);
  return true;
}

bool MyGetCurrentDirectory(CSysString &resultPath)
{
  DWORD needLength = ::GetCurrentDirectory(MAX_PATH + 1, 
      resultPath.GetBuffer(MAX_PATH));
  resultPath.ReleaseBuffer();
  return (needLength > 0 && needLength < MAX_PATH);
}
#endif

bool MySearchPath(LPCTSTR path, LPCTSTR fileName, LPCTSTR extension, 
  CSysString &resultPath, UINT32 &filePart)
{
  LPTSTR filePartPointer;
  DWORD value = ::SearchPath(path, fileName, extension, 
    MAX_PATH, resultPath.GetBuffer(MAX_PATH), &filePartPointer);
  filePart = filePartPointer - (LPCTSTR)resultPath;
  resultPath.ReleaseBuffer();
  if (value == 0 || value > MAX_PATH)
    return false;
  return true;
}


bool MyGetTempPath(CSysString &resultPath)
{
  DWORD needLength = ::GetTempPath(MAX_PATH + 1, 
      resultPath.GetBuffer(MAX_PATH));
  resultPath.ReleaseBuffer();
  return (needLength > 0 && needLength <= MAX_PATH);
}

UINT MyGetTempFileName(LPCTSTR dirPath, LPCTSTR prefix, CSysString &resultPath)
{
  UINT number = ::GetTempFileName(dirPath, prefix, 0,
      resultPath.GetBuffer(MAX_PATH));
  resultPath.ReleaseBuffer();
  return number;
}

UINT CTempFile::Create(LPCTSTR dirPath, LPCTSTR prefix, 
    CSysString &resultPath)
{
  Remove();
  UINT number = MyGetTempFileName(dirPath, prefix, resultPath);
  if(number != 0)
  {
    _fileName = resultPath;
    _mustBeDeleted = true;
  }
  return number;
}

bool CTempFile::Create(LPCTSTR prefix, CSysString &resultPath)
{
  CSysString tempPath;
  if(!MyGetTempPath(tempPath))
    return false;
  if (Create(tempPath, prefix, resultPath) != 0)
    return true;
  if(!NSystem::MyGetWindowsDirectory(tempPath))
    return false;
  return (Create(tempPath, prefix, resultPath) != 0);
}

bool CTempFile::Remove()
{
  if (!_mustBeDeleted)
    return true;
  _mustBeDeleted = !DeleteFileAlways(_fileName);
  return !_mustBeDeleted;
}

bool CreateTempDirectory(LPCTSTR prefix, CSysString &dirName)
{
  /*
  CSysString prefix = tempPath + prefixChars;
  CRandom random;
  random.Init();
  */
  while(true)
  {
    CTempFile tempFile;
    if (!tempFile.Create(prefix, dirName))
      return false;
    if (!::DeleteFile(dirName))
      return false;
    /*
    UINT32 randomNumber = random.Generate();
    TCHAR randomNumberString[32];
    _stprintf(randomNumberString, _T("%04X"), randomNumber);
    dirName = prefix + randomNumberString;
    */
    if(NFile::NFind::DoesFileExist(dirName))
      continue;
    bool result = NFile::NDirectory::MyCreateDirectory(dirName);
    if (result)
      return true;
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }
}

bool CTempDirectory::Create(LPCTSTR prefix)
{ 
  Remove();
  return (_mustBeDeleted = CreateTempDirectory(prefix, _tempDir)); 
}


}}}
