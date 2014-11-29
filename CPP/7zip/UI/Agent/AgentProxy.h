// AgentProxy.h

#ifndef __AGENT_PROXY_H
#define __AGENT_PROXY_H

#include "../Common/OpenArchive.h"

struct CProxyFile
{
  UInt32 Index;
  UString Name;
};

class CProxyFolder: public CProxyFile
{
public:
  int Parent;
  CRecordVector<unsigned> Folders;
  CObjectVector<CProxyFile> Files;
  UInt64 Size;
  UInt64 PackSize;
  UInt32 Crc;
  UInt32 NumSubFolders;
  UInt32 NumSubFiles;
  bool IsLeaf;
  bool CrcIsDefined;

  CProxyFolder(): Parent(-1) {};
  void AddFileSubItem(UInt32 index, const UString &name);
  void Clear();
};

class CProxyArchive
{
  int FindDirSubItemIndex(unsigned folderIndex, const UString &name, unsigned &insertPos) const;

  void CalculateSizes(unsigned folderIndex, IInArchive *archive);
  unsigned AddDirSubItem(unsigned folderIndex, UInt32 index, bool leaf, const UString &name);
public:
  CObjectVector<CProxyFolder> Folders; // Folders[0] - isRoot

  int FindDirSubItemIndex(unsigned folderIndex, const UString &name) const;
  void GetPathParts(int folderIndex, UStringVector &pathParts) const;
  UString GetFullPathPrefix(int folderIndex) const;
  
  // AddRealIndices DOES ADD also item represented by folderIndex (if it's Leaf)
  void AddRealIndices(unsigned folderIndex, CUIntVector &realIndices) const;
  int GetRealIndex(unsigned folderIndex, unsigned index) const;
  void GetRealIndices(unsigned folderIndex, const UInt32 *indices, UInt32 numItems, CUIntVector &realIndices) const;

  HRESULT Load(const CArc &arc, IProgress *progress);
};


// ---------- for Tree-mode archive ----------

struct CProxyFile2
{
  int FolderIndex;            // >= 0 for dir. (index in ProxyArchive2->Folders)
  int AltStreamsFolderIndex;  // >= 0 if there are alt streams. (index in ProxyArchive2->Folders)
  int Parent;                 // >= 0 if there is parent. (index in archive and in ProxyArchive2->Files)
  const wchar_t *Name;
  unsigned NameSize;
  bool Ignore;
  bool IsAltStream;
  bool NeedDeleteName;
  
  int GetFolderIndex(bool forAltStreams) const { return forAltStreams ? AltStreamsFolderIndex : FolderIndex; }

  bool IsDir() const { return FolderIndex >= 0; }
  CProxyFile2(): FolderIndex(-1), AltStreamsFolderIndex(-1), Name(NULL), Ignore(false), IsAltStream(false), NeedDeleteName(false) {}
  ~CProxyFile2()
  {
    if (NeedDeleteName)
      delete [](wchar_t *)Name;
  }
};

class CProxyFolder2
{
public:
  Int32 ArcIndex; // = -1 for Root folder
  CRecordVector<unsigned> SubFiles;
  UString PathPrefix;
  UInt64 Size;
  UInt64 PackSize;
  bool CrcIsDefined;
  UInt32 Crc;
  UInt32 NumSubFolders;
  UInt32 NumSubFiles;

  CProxyFolder2(): ArcIndex(-1) {};
  void AddFileSubItem(UInt32 index, const UString &name);
  void Clear();

};

class CProxyArchive2
{
  void CalculateSizes(unsigned folderIndex, IInArchive *archive);
  // AddRealIndices_of_Folder DOES NOT ADD item itself represented by folderIndex
  void AddRealIndices_of_Folder(unsigned folderIndex, bool includeAltStreams, CUIntVector &realIndices) const;
public:
  CObjectVector<CProxyFolder2> Folders; // Folders[0] - is root folder
  CObjArray<CProxyFile2> Files;  // all aitems from archive in same order

  bool IsThere_SubDir(unsigned folderIndex, const UString &name) const;

  void GetPathParts(int folderIndex, UStringVector &pathParts) const;
  UString GetFullPathPrefix(unsigned folderIndex) const;
  
  // AddRealIndices_of_ArcItem DOES ADD item and subItems
  void AddRealIndices_of_ArcItem(unsigned arcIndex, bool includeAltStreams, CUIntVector &realIndices) const;
  unsigned GetRealIndex(unsigned folderIndex, unsigned index) const;
  void GetRealIndices(unsigned folderIndex, const UInt32 *indices, UInt32 numItems, bool includeAltStreams, CUIntVector &realIndices) const;

  HRESULT Load(const CArc &arc, IProgress *progress);

  int GetParentFolderOfFile(UInt32 indexInArc) const
  {
    const CProxyFile2 &file = Files[indexInArc];
    if (file.Parent < 0)
      return 0;
    return Files[file.Parent].FolderIndex;
  }
};

#endif
