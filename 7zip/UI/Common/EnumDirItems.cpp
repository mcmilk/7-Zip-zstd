// EnumDirItems.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"
#include "Common/MyCom.h"

#include "EnumDirItems.h"

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

static HRESULT EnumerateDirItems(
    const NWildcard::CCensorNode &curNode, 
    const UString &diskPrefix, 
    const UString &archivePrefix, 
    const UString &addArchivePrefix, 
    CObjectVector<CDirItem> &dirItems, 
    bool enterToSubFolders,
    IEnumDirItemCallback *callback)
{
  if (!enterToSubFolders)
    if (curNode.NeedCheckSubDirs())
      enterToSubFolders = true;
  if (callback)
    RINOK(callback->CheckBreak());
  NFind::CEnumeratorW enumerator(diskPrefix + wchar_t(kAnyStringWildcard));
  NFind::CFileInfoW fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (callback)
      RINOK(callback->CheckBreak());
    UString name = fileInfo.Name;
    bool enterToSubFolders2 = enterToSubFolders;
    if (curNode.CheckPathToRoot(addArchivePrefix + name, !fileInfo.IsDirectory()))
    {
      AddDirFileInfo(archivePrefix, diskPrefix + fileInfo.Name, fileInfo, dirItems);
      if (fileInfo.IsDirectory())
        enterToSubFolders2 = true;;
    }
    if (!fileInfo.IsDirectory())
      continue;

    const NWildcard::CCensorNode *nextNode = 0;
    if (addArchivePrefix.IsEmpty())
    {
      int index = curNode.FindSubNode(name);
      if (index >= 0)
        nextNode = &curNode.SubNodes[index];
    }
    if (!enterToSubFolders2 && nextNode == 0)
      continue;

    UString archivePrefixNew = archivePrefix;
    UString addArchivePrefixNew = addArchivePrefix;
    if (nextNode == 0)
    {
      nextNode = &curNode;
      addArchivePrefixNew += name;
      addArchivePrefixNew += wchar_t(kDirDelimiter);
    }
    archivePrefixNew += name;
    archivePrefixNew += wchar_t(kDirDelimiter);
    RINOK(EnumerateDirItems(*nextNode,   
        diskPrefix + fileInfo.Name + wchar_t(kDirDelimiter), 
        archivePrefixNew, addArchivePrefixNew, 
        dirItems, enterToSubFolders2, callback));
  }
  return S_OK;
}

HRESULT EnumerateItems(const NWildcard::CCensor &censor, 
    CObjectVector<CDirItem> &dirItems, IEnumDirItemCallback *callback)
{
  for (int i = 0; i < censor.Pairs.Size(); i++)
  {
    if (callback)
      RINOK(callback->CheckBreak());
    const NWildcard::CPair &pair = censor.Pairs[i];
    RINOK(EnumerateDirItems(pair.Head, pair.Prefix, L"", L"", dirItems, false, callback));
  }
  return S_OK;
}
