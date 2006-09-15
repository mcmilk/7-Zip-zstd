// EnumDirItems.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Wildcard.h"
#include "Common/MyCom.h"

#include "EnumDirItems.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

void AddDirFileInfo(
    const UString &prefix,        // prefix for logical path
    const UString &fullPathName,  // path on disk: can be relative to some basePrefix
    const NFind::CFileInfoW &fileInfo, 
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
    const UString &baseFolderPrefix,  // base (disk) prefix for scanning  
    const UString &directory,         // additional disk prefix starting from baseFolderPrefix
    const UString &prefix,            // logical prefix
    CObjectVector<CDirItem> &dirItems,
    UStringVector &errorPaths,
    CRecordVector<DWORD> &errorCodes)
{
  NFind::CEnumeratorW enumerator(baseFolderPrefix + directory + wchar_t(kAnyStringWildcard));
  for (;;)
  { 
    NFind::CFileInfoW fileInfo;
    bool found;
    if (!enumerator.Next(fileInfo, found))
    {
      errorCodes.Add(::GetLastError());
      errorPaths.Add(baseFolderPrefix + directory);
      return;
    }
    if (!found)
      break;
    AddDirFileInfo(prefix, directory + fileInfo.Name, fileInfo, dirItems);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, directory + fileInfo.Name + wchar_t(kDirDelimiter), 
          prefix + fileInfo.Name + wchar_t(kDirDelimiter), dirItems, errorPaths, errorCodes);
    }
  }
}

void EnumerateDirItems(
    const UString &baseFolderPrefix,   // base (disk) prefix for scanning  
    const UStringVector &fileNames,    // names relative to baseFolderPrefix
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems,
    UStringVector &errorPaths,
    CRecordVector<DWORD> &errorCodes)
{
  for(int i = 0; i < fileNames.Size(); i++)
  {
    const UString &fileName = fileNames[i];
    NFind::CFileInfoW fileInfo;
    if (!NFind::FindFile(baseFolderPrefix + fileName, fileInfo))
    {
      errorCodes.Add(::GetLastError());
      errorPaths.Add(baseFolderPrefix + fileName);
      continue;
    }
    AddDirFileInfo(archiveNamePrefix, fileName, fileInfo, dirItems);
    if (fileInfo.IsDirectory())
    {
      EnumerateDirectory(baseFolderPrefix, fileName + wchar_t(kDirDelimiter), 
          archiveNamePrefix + fileInfo.Name +  wchar_t(kDirDelimiter), 
          dirItems, errorPaths, errorCodes);
    }
  }
}

static HRESULT EnumerateDirItems(
    const NWildcard::CCensorNode &curNode, 
    const UString &diskPrefix,        // full disk path prefix 
    const UString &archivePrefix,     // prefix from root
    const UStringVector &addArchivePrefix,  // prefix from curNode
    CObjectVector<CDirItem> &dirItems, 
    bool enterToSubFolders,
    IEnumDirItemCallback *callback,
    UStringVector &errorPaths,
    CRecordVector<DWORD> &errorCodes)
{
  if (!enterToSubFolders)
    if (curNode.NeedCheckSubDirs())
      enterToSubFolders = true;
  if (callback)
    RINOK(callback->CheckBreak());

  // try direct_names case at first
  if (addArchivePrefix.IsEmpty() && !enterToSubFolders)
  {
    // check that all names are direct
    int i;
    for (i = 0; i < curNode.IncludeItems.Size(); i++)
    {
      const NWildcard::CItem &item = curNode.IncludeItems[i];
      if (item.Recursive || item.PathParts.Size() != 1)
        break;
      const UString &name = item.PathParts.Front();
      if (name.IsEmpty() || DoesNameContainWildCard(name))
        break;
    }
    if (i == curNode.IncludeItems.Size())
    {
      // all names are direct (no wildcards)
      // so we don't need file_system's dir enumerator
      CRecordVector<bool> needEnterVector;
      for (i = 0; i < curNode.IncludeItems.Size(); i++)
      {
        const NWildcard::CItem &item = curNode.IncludeItems[i];
        const UString &name = item.PathParts.Front();
        const UString fullPath = diskPrefix + name;
        NFind::CFileInfoW fileInfo;
        if (!NFind::FindFile(fullPath, fileInfo))
        {
          errorCodes.Add(::GetLastError());
          errorPaths.Add(fullPath);
          continue;
        }
        bool isDir = fileInfo.IsDirectory();
        if (isDir && !item.ForDir || !isDir && !item.ForFile)
        {
          errorCodes.Add((DWORD)E_FAIL);
          errorPaths.Add(fullPath);
          continue;
        }
        const UString realName = fileInfo.Name;
        const UString realDiskPath = diskPrefix + realName;
        {
          UStringVector pathParts;
          pathParts.Add(fileInfo.Name);
          if (curNode.CheckPathToRoot(false, pathParts, !isDir))
            continue;
        }
        AddDirFileInfo(archivePrefix, realDiskPath, fileInfo, dirItems);
        if (!isDir)
          continue;
        
        UStringVector addArchivePrefixNew;
        const NWildcard::CCensorNode *nextNode = 0;
        int index = curNode.FindSubNode(name);
        if (index >= 0)
        {
          for (int t = needEnterVector.Size(); t <= index; t++)
            needEnterVector.Add(true);
          needEnterVector[index] = false;
          nextNode = &curNode.SubNodes[index];
        }
        else
        {
          nextNode = &curNode;
          addArchivePrefixNew.Add(name); // don't change it to realName. It's for shortnames support
        }
        RINOK(EnumerateDirItems(*nextNode,   
            realDiskPath + wchar_t(kDirDelimiter), 
            archivePrefix + realName + wchar_t(kDirDelimiter), 
            addArchivePrefixNew, dirItems, true, callback, errorPaths, errorCodes));
      }
      for (i = 0; i < curNode.SubNodes.Size(); i++)
      {
        if (i < needEnterVector.Size())
          if (!needEnterVector[i])
            continue;
        const NWildcard::CCensorNode &nextNode = curNode.SubNodes[i];
        const UString fullPath = diskPrefix + nextNode.Name;
        NFind::CFileInfoW fileInfo;
        if (!NFind::FindFile(fullPath, fileInfo))
        {
          if (!nextNode.AreThereIncludeItems())
            continue;
          errorCodes.Add(::GetLastError());
          errorPaths.Add(fullPath);
          continue;
        }
        if (!fileInfo.IsDirectory())
        {
          errorCodes.Add((DWORD)E_FAIL);
          errorPaths.Add(fullPath);
          continue;
        }
        RINOK(EnumerateDirItems(nextNode, 
            diskPrefix + fileInfo.Name + wchar_t(kDirDelimiter), 
            archivePrefix + fileInfo.Name + wchar_t(kDirDelimiter), 
            UStringVector(), dirItems, false, callback, errorPaths, errorCodes));
      }
      return S_OK;
    }
  }


  NFind::CEnumeratorW enumerator(diskPrefix + wchar_t(kAnyStringWildcard));
  for (;;)
  {
    NFind::CFileInfoW fileInfo;
    bool found;
    if (!enumerator.Next(fileInfo, found))
    {
      errorCodes.Add(::GetLastError());
      errorPaths.Add(diskPrefix);
      break;
    }
    if (!found)
      break;

    if (callback)
      RINOK(callback->CheckBreak());
    const UString &name = fileInfo.Name;
    bool enterToSubFolders2 = enterToSubFolders;
    UStringVector addArchivePrefixNew = addArchivePrefix;
    addArchivePrefixNew.Add(name);
    {
      UStringVector addArchivePrefixNewTemp(addArchivePrefixNew);
      if (curNode.CheckPathToRoot(false, addArchivePrefixNewTemp, !fileInfo.IsDirectory()))
        continue;
    }
    if (curNode.CheckPathToRoot(true, addArchivePrefixNew, !fileInfo.IsDirectory()))
    {
      AddDirFileInfo(archivePrefix, diskPrefix + name, fileInfo, dirItems);
      if (fileInfo.IsDirectory())
        enterToSubFolders2 = true;
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

    addArchivePrefixNew = addArchivePrefix;
    if (nextNode == 0)
    {
      nextNode = &curNode;
      addArchivePrefixNew.Add(name);
    }
    RINOK(EnumerateDirItems(*nextNode,   
        diskPrefix + name + wchar_t(kDirDelimiter), 
        archivePrefix + name + wchar_t(kDirDelimiter), 
        addArchivePrefixNew, dirItems, enterToSubFolders2, callback, errorPaths, errorCodes));
  }
  return S_OK;
}

HRESULT EnumerateItems(
    const NWildcard::CCensor &censor, 
    CObjectVector<CDirItem> &dirItems, 
    IEnumDirItemCallback *callback,
    UStringVector &errorPaths,
    CRecordVector<DWORD> &errorCodes)
{
  for (int i = 0; i < censor.Pairs.Size(); i++)
  {
    const NWildcard::CPair &pair = censor.Pairs[i];
    RINOK(EnumerateDirItems(pair.Head, pair.Prefix, L"", UStringVector(), dirItems, false, 
        callback, errorPaths, errorCodes));
  }
  return S_OK;
}
