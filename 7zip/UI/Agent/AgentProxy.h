// AgentProxy.h

#pragma once

#ifndef __AGENT_PROXY_H
#define __AGENT_PROXY_H

#include "Common/String.h"

#include "../../Archive/IArchive.h"

class CProxyFile
{
public:
  UINT32 Index;
  UString Name;
};

class CProxyFolder: public CProxyFile
{
public:
  CProxyFolder *Parent;
  CObjectVector<CProxyFolder> Folders;
  CObjectVector<CProxyFile> Files;
  bool IsLeaf;

  CProxyFolder(): Parent(NULL) {};
  int FindDirSubItemIndex(const UString &name, int &insertPos) const;
  int FindDirSubItemIndex(const UString &name) const;
  CProxyFolder* AddDirSubItem(UINT32 index, 
      bool leaf, const UString &name);
  void AddFileSubItem(UINT32 index, const UString &name);
  void Clear();

  UString GetFullPathPrefix() const;
  UString GetItemName(UINT32 index) const;
  void AddRealIndices(CUIntVector &realIndices) const;
  void GetRealIndices(const UINT32 *indices, UINT32 numItems, 
      CUIntVector &realIndices) const;
};

class CProxyArchive
{
  HRESULT ReadObjects(IInArchive *inArchive, IProgress *progress);
public:
  UString DefaultName;
  // FILETIME DefaultTime;
  // UINT32 DefaultAttributes;
  CProxyFolder RootFolder;
  HRESULT Reload(IInArchive *archive, IProgress *progress);
  HRESULT Load(IInArchive *archive, 
      const UString &defaultName,
      // const FILETIME &defaultTime,
      // UINT32 defaultAttributes,
      IProgress *progress);
};

#endif