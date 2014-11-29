// Windows/FileDir.h

#ifndef __WINDOWS_FILE_DIR_H
#define __WINDOWS_FILE_DIR_H

#include "../Common/MyString.h"

#include "FileIO.h"

namespace NWindows {
namespace NFile {
namespace NDir {

bool GetWindowsDir(FString &path);
bool GetSystemDir(FString &path);

bool SetDirTime(CFSTR path, const FILETIME *cTime, const FILETIME *aTime, const FILETIME *mTime);
bool SetFileAttrib(CFSTR path, DWORD attrib);
bool MyMoveFile(CFSTR existFileName, CFSTR newFileName);

#ifndef UNDER_CE
bool MyCreateHardLink(CFSTR newFileName, CFSTR existFileName);
#endif

bool RemoveDir(CFSTR path);
bool CreateDir(CFSTR path);
bool CreateComplexDir(CFSTR path);
bool DeleteFileAlways(CFSTR name);
bool RemoveDirWithSubItems(const FString &path);

bool MyGetFullPathName(CFSTR path, FString &resFullPath);
bool GetFullPathAndSplit(CFSTR path, FString &resDirPrefix, FString &resFileName);
bool GetOnlyDirPrefix(CFSTR path, FString &resDirPrefix);

#ifndef UNDER_CE

bool SetCurrentDir(CFSTR path);
bool GetCurrentDir(FString &resultPath);

#endif

bool MyGetTempPath(FString &resultPath);

class CTempFile
{
  bool _mustBeDeleted;
  FString _path;
  void DisableDeleting() { _mustBeDeleted = false; }
public:
  CTempFile(): _mustBeDeleted(false) {}
  ~CTempFile() { Remove(); }
  const FString &GetPath() const { return _path; }
  bool Create(CFSTR pathPrefix, NIO::COutFile *outFile); // pathPrefix is not folder prefix
  bool CreateRandomInTempFolder(CFSTR namePrefix, NIO::COutFile *outFile);
  bool Remove();
  bool MoveTo(CFSTR name, bool deleteDestBefore);
};

class CTempDir
{
  bool _mustBeDeleted;
  FString _path;
public:
  CTempDir(): _mustBeDeleted(false) {}
  ~CTempDir() { Remove();  }
  const FString &GetPath() const { return _path; }
  void DisableDeleting() { _mustBeDeleted = false; }
  bool Create(CFSTR namePrefix) ;
  bool Remove();
};

#if !defined(UNDER_CE)
class CCurrentDirRestorer
{
  FString _path;
public:
  bool NeedRestore;

  CCurrentDirRestorer(): NeedRestore(true)
  {
    GetCurrentDir(_path);
  }
  ~CCurrentDirRestorer()
  {
    if (!NeedRestore)
      return;
    FString s;
    if (GetCurrentDir(s))
      if (s != _path)
        SetCurrentDir(_path);
  }
};
#endif

}}}

#endif
