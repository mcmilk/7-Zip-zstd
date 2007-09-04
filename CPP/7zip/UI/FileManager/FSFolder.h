// FSFolder.h

#ifndef __FSFOLDER_H
#define __FSFOLDER_H

#include "Common/MyString.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "IFolder.h"

#include "TextPairs.h"

namespace NFsFolder {

class CFSFolder;

struct CFileInfoEx: public NWindows::NFile::NFind::CFileInfoW
{
  bool CompressedSizeIsDefined;
  UInt64 CompressedSize;
};

struct CDirItem;

struct CDirItem: public CFileInfoEx
{
  CDirItem *Parent;
  CObjectVector<CDirItem> Files;

  CDirItem(): Parent(0) {}
  void Clear()
  {
    Files.Clear();
    Parent = 0;
  }
};

class CFSFolder: 
  public IFolderFolder,
  public IFolderWasChanged,
  public IFolderOperations,
  // public IFolderOperationsDeleteToRecycleBin,
  public IFolderGetItemFullSize,
  public IFolderClone,
  public IFolderGetSystemIconIndex,
  public IFolderSetFlatMode,
  public CMyUnknownImp
{
  UInt64 GetSizeOfItem(int anIndex) const;
public:
  MY_QUERYINTERFACE_BEGIN
    MY_QUERYINTERFACE_ENTRY(IFolderWasChanged)
    // MY_QUERYINTERFACE_ENTRY(IFolderOperationsDeleteToRecycleBin)
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderGetItemFullSize)
    MY_QUERYINTERFACE_ENTRY(IFolderClone)
    MY_QUERYINTERFACE_ENTRY(IFolderGetSystemIconIndex)
    MY_QUERYINTERFACE_ENTRY(IFolderSetFlatMode)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE


  INTERFACE_FolderFolder(;)
  INTERFACE_FolderOperations(;)

  STDMETHOD(WasChanged)(INT32 *wasChanged);
  STDMETHOD(Clone)(IFolderFolder **resultFolder);
  STDMETHOD(GetItemFullSize)(UInt32 index, PROPVARIANT *value, IProgress *progress);

  STDMETHOD(SetFlatMode)(Int32 flatMode);

  STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  UString _path;
  CDirItem _root;
  CRecordVector<CDirItem *> _refs;

  CMyComPtr<IFolderFolder> _parentFolder;

  bool _commentsAreLoaded;
  CPairsStorage _comments;

  bool _flatMode;

  NWindows::NFile::NFind::CFindChangeNotification _findChangeNotification;

  HRESULT GetItemsFullSize(const UInt32 *indices, UInt32 numItems, 
      UInt64 &numFolders, UInt64 &numFiles, UInt64 &size, IProgress *progress);
  HRESULT GetItemFullSize(int index, UInt64 &size, IProgress *progress);
  HRESULT GetComplexName(const wchar_t *name, UString &resultPath);
  HRESULT BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder);

  bool LoadComments();
  bool SaveComments();
  HRESULT LoadSubItems(CDirItem &dirItem, const UString &path);
  void AddRefs(CDirItem &dirItem);
public:
  HRESULT Init(const UString &path, IFolderFolder *parentFolder);

  CFSFolder() : _flatMode(false) {}

  UString GetPrefix(const CDirItem &item) const;
  UString GetRelPath(const CDirItem &item) const;
  UString GetRelPath(UInt32 index) const { return GetRelPath(*_refs[index]); }

  void Clear()
  {
    _root.Clear();
    _refs.Clear();
  }
};

HRESULT GetFolderSize(const UString &path, UInt64 &numFolders, UInt64 &numFiles, UInt64 &size, IProgress *progress);

}

#endif
