// Windows/FileDir.h

#pragma once

#ifndef __WINDOWS_FILEDIR_H
#define __WINDOWS_FILEDIR_H

#include "Common/String.h"

namespace NWindows {
namespace NFile {
namespace NDirectory {

bool MyCreateDirectory(LPCTSTR aPathName);
bool CreateComplexDirectory(LPCTSTR aPathName);

bool DeleteFileAlways(LPCTSTR aName);
bool RemoveDirectoryWithSubItems(const CSysString &aPath);

#ifndef _WIN32_WCE
bool MyGetFullPathName(LPCTSTR aFileName, CSysString &aResultPath, 
    int &aFileNamePartStartIndex);
bool MyGetFullPathName(LPCTSTR aFileName, CSysString &aResultPath);
bool GetOnlyName(LPCTSTR aFileName, CSysString &aResultName);
bool GetOnlyDirPrefix(LPCTSTR aFileName, CSysString &aResultName);

bool MyGetCurrentDirectory(CSysString &aResultPath);
#endif

bool MySearchPath(LPCTSTR aPath, LPCTSTR aFileName, LPCTSTR anExtension, 
  CSysString &aResultPath, UINT32 &aFilePart);

inline bool MySearchPath(LPCTSTR aPath, LPCTSTR aFileName, LPCTSTR anExtension, 
  CSysString &aResultPath)
{
  UINT32 aValue;
  return MySearchPath(aPath, aFileName, anExtension, aResultPath, aValue);
}



bool MyGetTempPath(CSysString &aResultPath);

UINT MyGetTempFileName(LPCTSTR aDirPath, LPCTSTR aPrefix, CSysString &aResultPath);

class CTempFile
{
  bool m_MustBeDeleted;
  CSysString m_FileName;
public:
  CTempFile(): m_MustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  void DisableDeleting() {  m_MustBeDeleted = false; }

  UINT Create(LPCTSTR aDirPath, LPCTSTR aPrefix, CSysString &aResultPath);
  bool Create(LPCTSTR aPrefix, CSysString &aResultPath);
  bool Remove();
};

bool CreateTempDirectory(LPCTSTR aPrefixChars, CSysString &aDirName);

class CTempDirectory
{
  bool m_MustBeDeleted;
  CSysString m_TempDir;
public:
  const CSysString &GetPath() const { return m_TempDir; }
  CTempDirectory(): m_MustBeDeleted(false) {}
  ~CTempDirectory() { Remove();  }
  bool Create(LPCTSTR aPrefix) ;
  bool Remove()
  {
    if (!m_MustBeDeleted)
      return true;
    m_MustBeDeleted = !NFile::NDirectory::RemoveDirectoryWithSubItems(m_TempDir);
    return (!m_MustBeDeleted);
  }
};

}}}

#endif
