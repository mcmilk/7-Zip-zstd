// EnumDirItems.cpp

#include "StdAfx.h"

#include "EnumDirItems.h"
#include "Common/StringConvert.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

// using namespace NUpdateArchive;

void AddDirFileInfo(
    const UString &prefix, 
    const CSysString &fullPathName,
    NFind::CFileInfo &fileInfo, 
    CObjectVector<CDirItem> &dirItems,
    UINT codePage)
{
  CDirItem item;
  item.Attributes = fileInfo.Attributes;
  item.Size = fileInfo.Size;
  item.CreationTime = fileInfo.CreationTime;
  item.LastAccessTime = fileInfo.LastAccessTime;
  item.LastWriteTime = fileInfo.LastWriteTime;
  item.Name = prefix + GetUnicodeString(fileInfo.Name, codePage);
  item.FullPath = fullPathName;
  dirItems.Add(item);
}

static void EnumerateDirectory(
    const CSysString &baseFolderPrefix,
    const CSysString &directory, 
    const UString &prefix,
    CObjectVector<CDirItem> &dirItems, 
    UINT codePage)
{
  NFind::CEnumerator enumerator(baseFolderPrefix + directory + TCHAR(kAnyStringWildcard));
  NFind::CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  { 
    AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
        dirItems, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, directory + fileInfo.Name + TCHAR(kDirDelimiter), 
          prefix + GetUnicodeString(fileInfo.Name, codePage) + wchar_t(kDirDelimiter), 
          dirItems, codePage);
    }
  }
}

void EnumerateDirItems(
    const CSysString &baseFolderPrefix,
    const CSysStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems, 
    UINT codePage)
{
  for(int i = 0; i < fileNames.Size(); i++)
  {
    const CSysString &fileName = fileNames[i];
    NFind::CFileInfo fileInfo;
    if (!NFind::FindFile(baseFolderPrefix + fileName, fileInfo))
      throw 1081736;
    AddDirFileInfo(archiveNamePrefix, fileName, fileInfo, dirItems, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, fileName + TCHAR(kDirDelimiter), 
          archiveNamePrefix + GetUnicodeString(fileInfo.Name, codePage) + 
          wchar_t(kDirDelimiter), 
          dirItems, codePage);
    }
  }
}

/*
void EnumerateItems(const CSysStringVector &filePaths,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirItems, UINT codePage)
{
  for(int i = 0; i < filePaths.Size(); i++)
  {
    const CSysString &filePath = filePaths[i];
    NFind::CFileInfo fileInfo;
    if (!NFind::FindFile(filePath, fileInfo))
      throw 1081736;
    AddDirFileInfo(archiveNamePrefix, filePath, fileInfo, dirItems, codePage);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(TEXT(""), filePath + TCHAR(kDirDelimiter), 
          archiveNamePrefix + GetUnicodeString(fileInfo.Name, codePage) + 
          wchar_t(kDirDelimiter), 
          dirItems, codePage);
    }
  }
}
*/

