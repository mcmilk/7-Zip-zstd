// CompressEngineCommo.cpp

#include "StdAfx.h"

#include "CompressEngineCommon.h"
#include "Common/StringConvert.h"


using namespace NWindows;
using namespace NFile;
using namespace NName;

using namespace NUpdateArchive;



void AddDirFileInfo(const UString &aPrefix, const CSysString &aFullPathName,
    NFind::CFileInfo &aFileInfo, CArchiveStyleDirItemInfoVector &aDirFileInfoVector,
    UINT aCodePage)
{
  CArchiveStyleDirItemInfo aDirFileInfo;
  aDirFileInfo.Attributes = aFileInfo.Attributes;
  aDirFileInfo.Size = aFileInfo.Size;
  // ::FileTimeToLocalFileTime(&aFileInfo.LastWriteTime, &aDirFileInfo.LastWriteTime);
  aDirFileInfo.CreationTime = aFileInfo.CreationTime;
  aDirFileInfo.LastAccessTime = aFileInfo.LastAccessTime;
  aDirFileInfo.LastWriteTime = aFileInfo.LastWriteTime;
  aDirFileInfo.Name = aPrefix + GetUnicodeString(aFileInfo.Name, aCodePage);
  aDirFileInfo.FullPathDiskName = aFullPathName;
  aDirFileInfoVector.Add(aDirFileInfo);
}

void EnumerateDirectory(const CSysString &aBaseFolderPrefix,
    const CSysString &aDirectory, const UString &aPrefix,
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector, UINT aCodePage)
{
  NFind::CEnumerator anEnumerate(aBaseFolderPrefix + aDirectory + TCHAR(kAnyStringWildcard));
  NFind::CFileInfo aFileInfo;
  while (anEnumerate.Next(aFileInfo))
  { 
    AddDirFileInfo(aPrefix, aDirectory + aFileInfo.Name, aFileInfo, 
        aDirFileInfoVector, aCodePage);
    if (aFileInfo.IsDirectory())
    {
      EnumerateDirectory(aBaseFolderPrefix, aDirectory + aFileInfo.Name + TCHAR(kDirDelimiter), 
          aPrefix + GetUnicodeString(aFileInfo.Name, aCodePage) + wchar_t(kDirDelimiter), 
          aDirFileInfoVector, aCodePage);
    }
  }
}

void EnumerateItems(const CSysString &aBaseFolderPrefix,
    const CSysStringVector &aFileNames,
    const UString &anArchiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector, UINT aCodePage)
{
  for(int i = 0; i < aFileNames.Size(); i++)
  {
    const CSysString &aFileName = aFileNames[i];
    NFind::CFileInfo aFileInfo;
    if (!NFind::FindFile(aBaseFolderPrefix + aFileName, aFileInfo))
      throw 1081736;
    AddDirFileInfo(anArchiveNamePrefix, aFileName, aFileInfo, aDirFileInfoVector, aCodePage);
    if (aFileInfo.IsDirectory())
    {
      EnumerateDirectory(aBaseFolderPrefix, aFileName + TCHAR(kDirDelimiter), 
          anArchiveNamePrefix + GetUnicodeString(aFileInfo.Name, aCodePage) + 
          wchar_t(kDirDelimiter), 
          aDirFileInfoVector, aCodePage);
    }
  }
}

const CActionSet kAddActionSet = 
{
  NPairAction::kCopy,
  NPairAction::kCopy,
  NPairAction::kCompress,
  NPairAction::kCompress,
  NPairAction::kCompress,
  NPairAction::kCompress,
  NPairAction::kCompress
};

const CActionSet kUpdateActionSet = 
{
  NPairAction::kCopy,
  NPairAction::kCopy,
  NPairAction::kCompress,
  NPairAction::kCopy,
  NPairAction::kCompress,
  NPairAction::kCopy,
  NPairAction::kCompress
};

const CActionSet kFreshActionSet = 
{
  NPairAction::kCopy,
  NPairAction::kCopy,
  NPairAction::kIgnore,
  NPairAction::kCopy,
  NPairAction::kCompress,
  NPairAction::kCopy,
  NPairAction::kCompress
};

const CActionSet kSynchronizeActionSet = 
{
  NPairAction::kCopy,
  NPairAction::kIgnore,
  NPairAction::kCompress,
  NPairAction::kCopy,
  NPairAction::kCompress,
  NPairAction::kCopy,
  NPairAction::kCompress,
};

const CActionSet kDeleteActionSet = 
{
  NPairAction::kCopy,
  NPairAction::kIgnore,
  NPairAction::kIgnore,
  NPairAction::kIgnore,
  NPairAction::kIgnore,
  NPairAction::kIgnore,
  NPairAction::kIgnore
};
