// CompressEngineCommo.cpp

#include "StdAfx.h"

#include "CompressEngineCommon.h"
#include "Common/StringConvert.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

using namespace NUpdateArchive;

void AddDirFileInfo(const UString &prefix, const CSysString &fullPathName,
    NFind::CFileInfo &fileInfo, CArchiveStyleDirItemInfoVector &dirFileInfoVector,
    UINT codePage)
{
  CArchiveStyleDirItemInfo dirFileInfo;
  dirFileInfo.Attributes = fileInfo.Attributes;
  dirFileInfo.Size = fileInfo.Size;
  dirFileInfo.CreationTime = fileInfo.CreationTime;
  dirFileInfo.LastAccessTime = fileInfo.LastAccessTime;
  dirFileInfo.LastWriteTime = fileInfo.LastWriteTime;
  dirFileInfo.Name = prefix + GetUnicodeString(fileInfo.Name, codePage);
  dirFileInfo.FullPathDiskName = fullPathName;
  dirFileInfoVector.Add(dirFileInfo);
}

static void EnumerateDirectory(const CSysString &baseFolderPrefix,
    const CSysString &directory, const UString &prefix,
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage)
{
  NFind::CEnumerator enumerator(baseFolderPrefix + directory + TCHAR(kAnyStringWildcard));
  NFind::CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  { 
    AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
        dirFileInfoVector, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, directory + fileInfo.Name + TCHAR(kDirDelimiter), 
          prefix + GetUnicodeString(fileInfo.Name, codePage) + wchar_t(kDirDelimiter), 
          dirFileInfoVector, codePage);
    }
  }
}

void EnumerateItems(const CSysString &baseFolderPrefix,
    const CSysStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage)
{
  for(int i = 0; i < fileNames.Size(); i++)
  {
    const CSysString &fileName = fileNames[i];
    NFind::CFileInfo fileInfo;
    if (!NFind::FindFile(baseFolderPrefix + fileName, fileInfo))
      throw 1081736;
    AddDirFileInfo(archiveNamePrefix, fileName, fileInfo, dirFileInfoVector, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, fileName + TCHAR(kDirDelimiter), 
          archiveNamePrefix + GetUnicodeString(fileInfo.Name, codePage) + 
          wchar_t(kDirDelimiter), 
          dirFileInfoVector, codePage);
    }
  }
}

/*
void EnumerateItems(const CSysStringVector &filePaths,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage)
{
  for(int i = 0; i < filePaths.Size(); i++)
  {
    const CSysString &filePath = filePaths[i];
    NFind::CFileInfo fileInfo;
    if (!NFind::FindFile(filePath, fileInfo))
      throw 1081736;
    AddDirFileInfo(archiveNamePrefix, filePath, fileInfo, dirFileInfoVector, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(TEXT(""), filePath + TCHAR(kDirDelimiter), 
          archiveNamePrefix + GetUnicodeString(fileInfo.Name, codePage) + 
          wchar_t(kDirDelimiter), 
          dirFileInfoVector, codePage);
    }
  }
}
*/

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
