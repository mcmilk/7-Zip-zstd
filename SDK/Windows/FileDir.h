// Windows/FileDir.h

#pragma once

#ifndef __WINDOWS_FILEDIR_H
#define __WINDOWS_FILEDIR_H

#include "Common/String.h"

namespace NWindows {
namespace NFile {
namespace NDirectory {

bool MyCreateDirectory(LPCTSTR pathName);
bool CreateComplexDirectory(LPCTSTR pathName);

bool DeleteFileAlways(LPCTSTR name);
bool RemoveDirectoryWithSubItems(const CSysString &path);

#ifndef _WIN32_WCE
bool MyGetShortPathName(LPCTSTR longPath, CSysString &shortPath);

bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath, 
    int &fileNamePartStartIndex);
bool MyGetFullPathName(LPCTSTR fileName, CSysString &resultPath);
bool GetOnlyName(LPCTSTR fileName, CSysString &resultName);
bool GetOnlyDirPrefix(LPCTSTR fileName, CSysString &resultName);

bool MyGetCurrentDirectory(CSysString &resultPath);
#endif

bool MySearchPath(LPCTSTR path, LPCTSTR fileName, LPCTSTR extension, 
  CSysString &resultPath, UINT32 &filePart);

inline bool MySearchPath(LPCTSTR path, LPCTSTR fileName, LPCTSTR extension, 
  CSysString &resultPath)
{
  UINT32 value;
  return MySearchPath(path, fileName, extension, resultPath, value);
}



bool MyGetTempPath(CSysString &resultPath);

UINT MyGetTempFileName(LPCTSTR dirPath, LPCTSTR prefix, CSysString &resultPath);

class CTempFile
{
  bool _mustBeDeleted;
  CSysString _fileName;
public:
  CTempFile(): _mustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  void DisableDeleting() {  _mustBeDeleted = false; }

  UINT Create(LPCTSTR dirPath, LPCTSTR prefix, CSysString &resultPath);
  bool Create(LPCTSTR prefix, CSysString &resultPath);
  bool Remove();
};

bool CreateTempDirectory(LPCTSTR prefixChars, CSysString &dirName);

class CTempDirectory
{
  bool _mustBeDeleted;
  CSysString _tempDir;
public:
  const CSysString &GetPath() const { return _tempDir; }
  CTempDirectory(): _mustBeDeleted(false) {}
  ~CTempDirectory() { Remove();  }
  bool Create(LPCTSTR prefix) ;
  bool Remove()
  {
    if (!_mustBeDeleted)
      return true;
    _mustBeDeleted = !NFile::NDirectory::RemoveDirectoryWithSubItems(_tempDir);
    return (!_mustBeDeleted);
  }
};

}}}

#endif
