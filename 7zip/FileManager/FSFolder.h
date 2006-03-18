// FSFolder.h

#ifndef __FSFOLDER_H
#define __FSFOLDER_H

#include "Common/String.h"
#include "Common/MyCom.h"
#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "IFolder.h"

#include "TextPairs.h"

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
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
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
    MY_QUERYINTERFACE_ENTRY(IEnumProperties)
    MY_QUERYINTERFACE_ENTRY(IFolderGetTypeID)
    MY_QUERYINTERFACE_ENTRY(IFolderGetPath)
    MY_QUERYINTERFACE_ENTRY(IFolderWasChanged)
    // MY_QUERYINTERFACE_ENTRY(IFolderOperationsDeleteToRecycleBin)
    MY_QUERYINTERFACE_ENTRY(IFolderOperations)
    MY_QUERYINTERFACE_ENTRY(IFolderGetItemFullSize)
    MY_QUERYINTERFACE_ENTRY(IFolderClone)
    MY_QUERYINTERFACE_ENTRY(IFolderGetSystemIconIndex)
    MY_QUERYINTERFACE_ENTRY(IFolderSetFlatMode)
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE


  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UInt32 *numItems);  
  STDMETHOD(GetProperty)(UInt32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UInt32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties);  
  STDMETHOD(GetPropertyInfo)(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType);
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);
  STDMETHOD(WasChanged)(INT32 *wasChanged);
  STDMETHOD(Clone)(IFolderFolder **resultFolder);
  STDMETHOD(GetItemFullSize)(UInt32 index, PROPVARIANT *value, IProgress *progress);

  STDMETHOD(SetFlatMode)(Int32 flatMode);

  // IFolderOperations

  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress);
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress);
  STDMETHOD(Rename)(UInt32 index, const wchar_t *newName, IProgress *progress);
  STDMETHOD(Delete)(const UInt32 *indices, UInt32 numItems, IProgress *progress);
  STDMETHOD(CopyTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(MoveTo)(const UInt32 *indices, UInt32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UInt32 numItems, IProgress *progress);
  STDMETHOD(SetProperty)(UInt32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress);
  STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  UString _path;
  CDirItem _root;
  CRecordVector<CDirItem *> _refs;

  CMyComPtr<IFolderFolder> _parentFolder;

  bool _findChangeNotificationDefined;

  bool _commentsAreLoaded;
  CPairsStorage _comments;

  bool _flatMode;

  NWindows::NFile::NFind::CFindChangeNotification _findChangeNotification;

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

#endif
