// UpdateUtils.cpp

#include "StdAfx.h"

#include "UpdateUtils.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;


CSysString GetContainingDir(const CSysString &aPath)
{
  CSysString aResultPath;
  int aPos;
  if(! NFile::NDirectory::MyGetFullPathName(aPath, aResultPath, aPos))
    throw 141716;
  return aResultPath.Left(aPos);
}


CSysString GetWorkDir(const NZipSettings::NWorkDir::CInfo &aWorkDirInfo,
    const CSysString &anArchiveName)
{
  NZipSettings::NWorkDir::NMode::EEnum aMode = aWorkDirInfo.Mode;
  if (aWorkDirInfo.ForRemovableOnly)
  {
    aMode = NZipSettings::NWorkDir::NMode::kCurrent;
    CSysString aPrefix = anArchiveName.Left(3);
    if (aPrefix[1] == TEXT(':') && aPrefix[2] == TEXT('\\'))
    {
      UINT aDriveType = GetDriveType(aPrefix);
      if (aDriveType == DRIVE_CDROM || aDriveType == DRIVE_REMOVABLE)
        aMode = aWorkDirInfo.Mode;
    }
    /*
    CParsedPath aParsedPath;
    aParsedPath.ParsePath(anArchiveName);
    UINT aDriveType = GetDriveType(aParsedPath.Prefix);
    if ((aDriveType != DRIVE_CDROM) && (aDriveType != DRIVE_REMOVABLE))
      aMode = NZipSettings::NWorkDir::NMode::kCurrent;
    */
  }
  switch(aMode)
  {
    case NZipSettings::NWorkDir::NMode::kCurrent:
    {
      return GetContainingDir(anArchiveName);
    }
    case NZipSettings::NWorkDir::NMode::kSpecified:
    {
      CSysString aTempDir = aWorkDirInfo.Path;
      NormalizeDirPathPrefix(aTempDir);
      return aTempDir;
    }
    default: // NZipSettings::NWorkDir::NMode::kSystem:
    {
      CSysString aTempDir;
      if(!NFile::NDirectory::MyGetTempPath(aTempDir))
        throw 141717;
      return aTempDir;
    }
  }
}



