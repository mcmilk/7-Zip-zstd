// WorkDir.cpp

#include "StdAfx.h"

#include "WorkDir.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

static CSysString GetContainingDir(const CSysString &aPath)
{
  CSysString resultPath;
  int pos;
  if(! NFile::NDirectory::MyGetFullPathName(aPath, resultPath, pos))
    throw 141716;
  return resultPath.Left(pos);
}

CSysString GetWorkDir(const NWorkDir::CInfo &workDirInfo,
    const CSysString &archiveName)
{
  NWorkDir::NMode::EEnum mode = workDirInfo.Mode;
  if (workDirInfo.ForRemovableOnly)
  {
    mode = NWorkDir::NMode::kCurrent;
    CSysString prefix = archiveName.Left(3);
    if (prefix[1] == TEXT(':') && prefix[2] == TEXT('\\'))
    {
      UINT driveType = GetDriveType(prefix);
      if (driveType == DRIVE_CDROM || driveType == DRIVE_REMOVABLE)
        mode = workDirInfo.Mode;
    }
    /*
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(archiveName);
    UINT driveType = GetDriveType(aParsedPath.Prefix);
    if ((driveType != DRIVE_CDROM) && (driveType != DRIVE_REMOVABLE))
      mode = NZipSettings::NWorkDir::NMode::kCurrent;
    */
  }
  switch(mode)
  {
    case NWorkDir::NMode::kCurrent:
    {
      return GetContainingDir(archiveName);
    }
    case NWorkDir::NMode::kSpecified:
    {
      CSysString tempDir = workDirInfo.Path;
      NormalizeDirPathPrefix(tempDir);
      return tempDir;
    }
    default: // NZipSettings::NWorkDir::NMode::kSystem:
    {
      CSysString tempDir;
      if(!NFile::NDirectory::MyGetTempPath(tempDir))
        throw 141717;
      return tempDir;
    }
  }
}



