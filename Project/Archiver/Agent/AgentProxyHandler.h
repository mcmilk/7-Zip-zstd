// ProxyHandler.h

#pragma once

#ifndef __AGENTPROXYHANDLER_H
#define __AGENTPROXYHANDLER_H

#include "Common/String.h"

#include "Windows/COM.h"
#include "Windows/FileFind.h"

#include "../Common/IArchiveHandler2.h"

class CFileItem
{
public:
  UINT32 m_Index;
  UString m_Name;
};

class CFolderItem: public CFileItem
{
public:
  CFolderItem *m_Parent;
  CObjectVector<CFolderItem> m_FolderSubItems;
  CObjectVector<CFileItem> m_FileSubItems;
  UINT32 m_Index;
  UString m_Name;
  bool m_IsLeaf;

  CFolderItem(): m_Parent(NULL) {};
  int FindDirSubItemIndex(const UString &aName, int &anInsertPos) const;
  int FindDirSubItemIndex(const UString &aName) const;
  CFolderItem* AddDirSubItem(UINT32 anIndex, 
      bool anLeaf, const UString &aName);
  void AddFileSubItem(UINT32 anIndex, const UString &aName);
  void Clear();
};

////////////////////////////////////////////////

class CAgentProxyHandler
{
  void ClearState();
  // HRESULT ReadProperties(IArchiveHandler200 *aHandler);
  HRESULT ReadObjects(IArchiveHandler200 *aHandler, IProgress *aProgress);
public:
  CComPtr<IArchiveHandler200> _archiveHandler;
  UString _itemDefaultName;
  FILETIME _defaultTime;
  UINT32 _defaultAttributes;

  // NWindows::NFile::NFind::CFileInfo m_ArchiveFileInfo;

  CFolderItem _folderItemHead;

  // CArchiveItemPropertyVector m_HandlerProperties;
  // CArchiveItemPropertyVector m_InternalProperties;

  HRESULT ReInit(IProgress *aProgress);
  HRESULT Init(IArchiveHandler200 *anArchiveHandler, 
      // const NWindows::NFile::NFind::CFileInfo &anArchiveFileInfo,
      const UString &anItemDefaultName,
      const FILETIME &aDefaultTime,
      UINT32 aDefaultAttributes,
      IProgress *aProgress);


  void AddRealIndexes(const CFolderItem &anItem, 
      std::vector<UINT32> &aRealIndexes);
  void GetRealIndexes(const CFolderItem &anItem, 
      const UINT32 *anIndexes, 
      UINT32 aNumItems, 
      std::vector<UINT32> &aRealIndexes);

};

#endif