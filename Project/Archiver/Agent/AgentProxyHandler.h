// ProxyHandler.h

#pragma once

#ifndef __AGENTPROXYHANDLER_H
#define __AGENTPROXYHANDLER_H

#include "Common/String.h"

#include "Windows/COM.h"
#include "Windows/FileFind.h"

#include "../Common/FolderArchiveInterface.h"

class CFileItem
{
public:
  UINT32 Index;
  UString Name;
};

class CFolderItem: public CFileItem
{
public:
  CFolderItem *Parent;
  CObjectVector<CFolderItem> FolderSubItems;
  CObjectVector<CFileItem> FileSubItems;
  UINT32 Index;
  UString Name;
  bool IsLeaf;

  CFolderItem(): Parent(NULL) {};
  int FindDirSubItemIndex(const UString &name, int &insertPos) const;
  int FindDirSubItemIndex(const UString &name) const;
  CFolderItem* AddDirSubItem(UINT32 index, 
      bool leaf, const UString &name);
  void AddFileSubItem(UINT32 index, const UString &name);
  void Clear();

  UString GetFullPathPrefix() const 
  {
    UString result;
    const CFolderItem *current = this;
    while (current->Parent != NULL)
    {
      result = current->Name + UString(L'\\') + result;
      current = current->Parent;
    }
    return result;
  }

  UString GetItemName(UINT32 index) const 
  {
    if (index < FolderSubItems.Size())
      return FolderSubItems[index].Name;
    return FileSubItems[index - FolderSubItems.Size()].Name;
  }

};

////////////////////////////////////////////////

class CAgentProxyHandler
{
  void ClearState();
  // HRESULT ReadProperties(IArchiveHandler200 *aHandler);
  HRESULT ReadObjects(IInArchive *aHandler, IProgress *progress);
public:
  CComPtr<IInArchive> Archive;
  UString ItemDefaultName;
  FILETIME DefaultTime;
  UINT32 DefaultAttributes;

  // NWindows::NFile::NFind::CFileInfo m_ArchiveFileInfo;

  CFolderItem FolderItemHead;

  // CArchiveItemPropertyVector m_HandlerProperties;
  // CArchiveItemPropertyVector m_InternalProperties;

  HRESULT ReInit(IProgress *progress);
  HRESULT Init(IInArchive *archive, 
      // const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
      const UString &itemDefaultName,
      const FILETIME &defaultTime,
      UINT32 defaultAttributes,
      IProgress *progress);


  void AddRealIndices(const CFolderItem &item, CUIntVector &realIndices);
  void GetRealIndices(const CFolderItem &item, const UINT32 *indices, 
      UINT32 numItems, CUIntVector &realIndices);

};

#endif