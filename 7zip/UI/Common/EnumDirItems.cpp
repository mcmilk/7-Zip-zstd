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
    const UString &fullPathName,
    NFind::CFileInfoW &fileInfo, 
    CObjectVector<CDirItem> &dirItems)
{
  CDirItem item;
  item.Attributes = fileInfo.Attributes;
  item.Size = fileInfo.Size;
  item.CreationTime = fileInfo.CreationTime;
  item.LastAccessTime = fileInfo.LastAccessTime;
  item.LastWriteTime = fileInfo.LastWriteTime;
  item.Name = prefix + fileInfo.Name;
  item.FullPath = fullPathName;
  dirItems.Add(item);
}

static void EnumerateDirectory(
    const UString &baseFolderPrefix,
    const UString &directory, 
    const UString &prefix,
    CObjectVector<CDirItem> &dirItems)
{
  NFind::CEnumeratorW enumerator(baseFolderPrefix + directory + wchar_t(kAnyStringWildcard));
  NFind::CFileInfoW fileInfo;
  while (enumerator.Next(fileInfo))
  { 
    AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, 
        dirItems);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, directory + fileInfo.Name + wchar_t(kDirDelimiter), 
          prefix + fileInfo.Name + wchar_t(kDirDelimiter), dirItems);
    }
  }
}

void EnumerateDirItems(
    const UString &baseFolderPrefix,
    const UStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems)
{
  for(int i = 0; i < fileNames.Size(); i++)
  {
    const UString &fileName = fileNames[i];
    NFind::CFileInfoW fileInfo;
    if (!NFind::FindFile(baseFolderPrefix + fileName, fileInfo))
      throw 1081736;
    AddDirFileInfo(archiveNamePrefix, fileName, fileInfo, dirItems);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, fileName + wchar_t(kDirDelimiter), 
          archiveNamePrefix + fileInfo.Name +  wchar_t(kDirDelimiter), 
          dirItems);
    }
  }
}
