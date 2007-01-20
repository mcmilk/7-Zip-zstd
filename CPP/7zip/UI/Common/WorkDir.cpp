// WorkDir.cpp

#include "StdAfx.h"

#include "WorkDir.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"

static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 

using namespace NWindows;
using namespace NFile;
using namespace NName;

UString GetWorkDir(const NWorkDir::CInfo &workDirInfo, const UString &path)
{
  NWorkDir::NMode::EEnum mode = workDirInfo.Mode;
  if (workDirInfo.ForRemovableOnly)
  {
    mode = NWorkDir::NMode::kCurrent;
    UString prefix = path.Left(3);
    if (prefix[1] == L':' && prefix[2] == L'\\')
    {
      UINT driveType = GetDriveType(GetSystemString(prefix, GetCurrentCodePage()));
      if (driveType == DRIVE_CDROM || driveType == DRIVE_REMOVABLE)
        mode = workDirInfo.Mode;
    }
    /*
    CParsedPath parsedPath;
    parsedPath.ParsePath(archiveName);
    UINT driveType = GetDriveType(parsedPath.Prefix);
    if ((driveType != DRIVE_CDROM) && (driveType != DRIVE_REMOVABLE))
      mode = NZipSettings::NWorkDir::NMode::kCurrent;
    */
  }
  switch(mode)
  {
    case NWorkDir::NMode::kCurrent:
    {
      return ExtractDirPrefixFromPath(path);
    }
    case NWorkDir::NMode::kSpecified:
    {
      UString tempDir = workDirInfo.Path;
      NormalizeDirPathPrefix(tempDir);
      return tempDir;
    }
    default:
    {
      UString tempDir;
      if(!NFile::NDirectory::MyGetTempPath(tempDir))
        throw 141717;
      return tempDir;
    }
  }
}



