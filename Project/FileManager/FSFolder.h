// FSFolder.h

#pragma once

#ifndef __FSFOLDER_H
#define __FSFOLDER_H

#include "Common/String.h"

#include "Windows/FileFind.h"
#include "Windows/PropVariant.h"

#include "FolderInterface.h"

#include "TextPairs.h"

class CFSFolder;

struct CFileInfoEx: public NWindows::NFile::NFind::CFileInfo
{
  bool CompressedSizeIsDefined;
  UINT64 CompressedSize;
};


class CFSFolder: 
  public IFolderFolder,
  public IEnumProperties,
  public IFolderGetTypeID,
  public IFolderGetPath,
  public IFolderWasChanged,
  public IFolderOperations,
  public IFolderGetItemFullSize,
  public IFolderClone,
  public IFolderGetSystemIconIndex,
  public CComObjectRoot
{
  UINT64 GetSizeOfItem(int anIndex) const;
public:
BEGIN_COM_MAP(CFSFolder)
  COM_INTERFACE_ENTRY(IFolderFolder)
  COM_INTERFACE_ENTRY(IEnumProperties)
  COM_INTERFACE_ENTRY(IFolderGetTypeID)
  COM_INTERFACE_ENTRY(IFolderGetPath)
  COM_INTERFACE_ENTRY(IFolderWasChanged)
  COM_INTERFACE_ENTRY(IFolderOperations)
  COM_INTERFACE_ENTRY(IFolderGetItemFullSize)
  COM_INTERFACE_ENTRY(IFolderClone)
  COM_INTERFACE_ENTRY(IFolderGetSystemIconIndex)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CFSFolder)

DECLARE_NO_REGISTRY()

  STDMETHOD(LoadItems)();
  STDMETHOD(GetNumberOfItems)(UINT32 *numItems);  
  STDMETHOD(GetProperty)(UINT32 itemIndex, PROPID propID, PROPVARIANT *value);
  STDMETHOD(BindToFolder)(UINT32 index, IFolderFolder **resultFolder);
  STDMETHOD(BindToFolder)(const wchar_t *name, IFolderFolder **resultFolder);
  STDMETHOD(BindToParentFolder)(IFolderFolder **resultFolder);
  STDMETHOD(GetName)(BSTR *name);

  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetTypeID)(BSTR *name);
  STDMETHOD(GetPath)(BSTR *path);
  STDMETHOD(WasChanged)(INT32 *wasChanged);
  STDMETHOD(Clone)(IFolderFolder **resultFolder);
  STDMETHOD(GetItemFullSize)(UINT32 index, PROPVARIANT *value, IProgress *progress);

  // IFolderOperations

  STDMETHOD(CreateFolder)(const wchar_t *name, IProgress *progress);
  STDMETHOD(CreateFile)(const wchar_t *name, IProgress *progress);
  STDMETHOD(Rename)(UINT32 index, const wchar_t *newName, IProgress *progress);
  STDMETHOD(Delete)(const UINT32 *indices, UINT32 numItems, IProgress *progress);
  STDMETHOD(CopyTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(MoveTo)(const UINT32 *indices, UINT32 numItems, 
      const wchar_t *path, IFolderOperationsExtractCallback *callback);
  STDMETHOD(CopyFrom)(const wchar_t *fromFolderPath,
      const wchar_t **itemsPaths, UINT32 numItems, IProgress *progress);
  STDMETHOD(SetProperty)(UINT32 index, PROPID propID, const PROPVARIANT *value, IProgress *progress);
  STDMETHOD(GetSystemIconIndex)(UINT32 index, INT32 *iconIndex);

private:
  UINT _fileCodePage;
  CSysString _path;
  CObjectVector<CFileInfoEx> _files;
  CComPtr<IFolderFolder> _parentFolder;

  bool _findChangeNotificationDefined;

  bool _commentsAreLoaded;
  CPairsStorage _comments;

  NWindows::NFile::NFind::CFindChangeNotification _findChangeNotification;

  HRESULT GetItemFullSize(int index, UINT64 &size, IProgress *progress);
  HRESULT GetComplexName(const wchar_t *name, CSysString &resultPath);
  HRESULT BindToFolderSpec(const TCHAR *name, IFolderFolder **resultFolder);

  bool LoadComments();
  bool SaveComments();
public:
  HRESULT Init(const CSysString &aPath, IFolderFolder *aParentFolder);
};

#endif
