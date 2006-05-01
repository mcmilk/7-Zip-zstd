// Agent.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "../Common/DefaultName.h"
#include "../Common/ArchiveExtractCallback.h"

#include "Agent.h"

#ifdef FORMAT_7Z
#include "../../Archive/7z/7zHandler.h"
#endif

using namespace NWindows;

STDMETHODIMP CAgentFolder::GetAgentFolder(CAgentFolder **agentFolder)
{ 
  *agentFolder = this; 
  return S_OK; 
}

void CAgentFolder::LoadFolder(CProxyFolder *folder)
{
  int i;
  CProxyItem item;
  item.Folder = folder;
  for (i = 0; i < folder->Folders.Size(); i++)
  {
    item.Index = i;
    _items.Add(item);
    LoadFolder(&folder->Folders[i]);
  }
  int start = folder->Folders.Size();
  for (i = 0; i < folder->Files.Size(); i++)
  {
    item.Index = start + i;
    _items.Add(item);
  }
}


STDMETHODIMP CAgentFolder::LoadItems()
{
  _items.Clear();
  if (_flatMode)
    LoadFolder(_proxyFolderItem);
  return S_OK;
}

STDMETHODIMP CAgentFolder::GetNumberOfItems(UINT32 *numItems)
{
  if (_flatMode)
    *numItems = _items.Size();
  else
    *numItems = _proxyFolderItem->Folders.Size() +_proxyFolderItem->Files.Size();
  return S_OK;
}

/*
STDMETHODIMP CAgentFolder::GetNumberOfSubFolders(UINT32 *aNumSubFolders)
{
  *aNumSubFolders = _proxyFolderItem->Folders.Size();
  return S_OK;
}
*/

UString CAgentFolder::GetName(UInt32 index) const
{
  UInt32 realIndex;
  const CProxyFolder *folder;
  if (_flatMode)
  {
    const CProxyItem &item = _items[index];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = index;
  }

  if (realIndex < (UINT32)folder->Folders.Size())
    return folder->Folders[realIndex].Name;
  return folder->Files[realIndex - folder->Folders.Size()].Name;
}

UString CAgentFolder::GetPrefix(UInt32 index) const
{
  if (!_flatMode)
    return UString();
  const CProxyItem &item = _items[index];
  const CProxyFolder *folder = item.Folder;
  UString path;
  while(folder != _proxyFolderItem)
  {
    path = folder->Name + UString(L"\\") + path;
    folder = folder->Parent;
  }
  return path;
}

UString CAgentFolder::GetFullPathPrefixPlusPrefix(UInt32 index) const
{
  return _proxyFolderItem->GetFullPathPrefix() + GetPrefix(index);
}

void CAgentFolder::GetPrefixIfAny(UInt32 index, NCOM::CPropVariant &propVariant) const
{
  if (!_flatMode)
    return;
  propVariant = GetPrefix(index);
}


STDMETHODIMP CAgentFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  const CProxyFolder *folder;
  UInt32 realIndex;
  if (_flatMode)
  {
    const CProxyItem &item = _items[itemIndex];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = itemIndex;
  }

  if (realIndex < (UINT32)folder->Folders.Size())
  {
    const CProxyFolder &item = folder->Folders[realIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = true;
        break;
      case kpidName:
        propVariant = item.Name;
        break;
      case kpidPrefix:
        GetPrefixIfAny(itemIndex, propVariant);
        break;
      default:
        if (item.IsLeaf)
          return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  else
  {
    realIndex -= folder->Folders.Size();
    const CProxyFile &item = folder->Files[realIndex];
    switch(propID)
    {
      case kpidIsFolder:
        propVariant = false;
        break;
      case kpidName:
        propVariant = item.Name;
        break;
      case kpidPrefix:
        GetPrefixIfAny(itemIndex, propVariant);
        break;
      default:
        return _agentSpec->GetArchive()->GetProperty(item.Index, propID, value);
    }
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT CAgentFolder::BindToFolder(CProxyFolder *folder, IFolderFolder **resultFolder)
{
  CMyComPtr<IFolderFolder> parentFolder;
  if (folder->Parent != _proxyFolderItem)
  {
    RINOK(BindToFolder(folder->Parent, &parentFolder));
  }
  else
    parentFolder = this;
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> agentFolder = folderSpec;
  folderSpec->Init(_proxyArchive, folder, parentFolder, _agentSpec);
  *resultFolder = agentFolder.Detach();
  return S_OK;
}

STDMETHODIMP CAgentFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN

  CProxyFolder *folder;
  UInt32 realIndex;
  if (_flatMode)
  {
    const CProxyItem &item = _items[index];
    folder = item.Folder;
    realIndex = item.Index;
  }
  else
  {
    folder = _proxyFolderItem;
    realIndex = index;
  }
  if (realIndex >= (UINT32)folder->Folders.Size())
    return E_INVALIDARG;
  return BindToFolder(&folder->Folders[realIndex], resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  int index = _proxyFolderItem->FindDirSubItemIndex(name);
  if (index < 0)
    return E_INVALIDARG;
  return BindToFolder(index, resultFolder);
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
  *resultFolder = parentFolder.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetName(BSTR *name)
{
  CMyComBSTR temp = _proxyFolderItem->Name;
  *name = temp.Detach();
  return S_OK;
}


#ifdef NEW_FOLDER_INTERFACE

struct CArchiveItemPropertyTemp
{
  UString Name;
  PROPID ID;
  VARTYPE Type;
};

STDMETHODIMP CAgentFolder::GetNumberOfProperties(UINT32 *numProperties)
{
  COM_TRY_BEGIN
  RINOK(_agentSpec->GetArchive()->GetNumberOfProperties(numProperties));
  if (_flatMode)
    (*numProperties)++;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  UINT32 numProperties;
  _agentSpec->GetArchive()->GetNumberOfProperties(&numProperties);
  if (index < numProperties)
  {
    RINOK(_agentSpec->GetArchive()->GetPropertyInfo(index, name, propID, varType));
    if (*propID == kpidPath)
      *propID = kpidName;
  }
  else
  {
    *name = NULL;
    *propID = kpidPrefix;
    *varType = VT_BSTR;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetTypeID(BSTR *name)
{
  COM_TRY_BEGIN
  UString temp = UString(L"7-Zip.") + _agentSpec->ArchiveType;
  CMyComBSTR bstrTemp = temp;
  *name = bstrTemp.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::GetPath(BSTR *path)
{
  COM_TRY_BEGIN
  UStringVector pathParts;
  pathParts.Clear();
  CMyComPtr<IFolderFolder> currentFolder = this;
  while (true)
  {
    CMyComPtr<IFolderFolder> newFolder;
    currentFolder->BindToParentFolder(&newFolder);  
    if (newFolder == NULL)
      break;
    CMyComBSTR aName;
    currentFolder->GetName(&aName);
    pathParts.Insert(0, (const wchar_t *)aName);
    currentFolder = newFolder;
  }

  UString prefix;
  for(int i = 0; i < pathParts.Size(); i++)
  {
    prefix += pathParts[i];
    prefix += L'\\';
  }

  CMyComBSTR tempPath = prefix;
  *path = tempPath.Detach();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgentFolder::SetFlatMode(Int32 flatMode)
{
  _flatMode = IntToBool(flatMode);
  return S_OK;
}

#endif

void CAgentFolder::GetRealIndices(const UINT32 *indices, UINT32 numItems, CUIntVector &realIndices) const
{
  if (!_flatMode)
  {
    _proxyFolderItem->GetRealIndices(indices, numItems, realIndices);
    return;
  }
  realIndices.Clear();
  for(UINT32 i = 0; i < numItems; i++)
  {
    const CProxyItem &item = _items[indices[i]];
    const CProxyFolder *folder = item.Folder;
    UInt32 realIndex = item.Index;
    if (realIndex < (UINT32)folder->Folders.Size())
      continue;
    realIndices.Add(folder->Files[realIndex - folder->Folders.Size()].Index);
  }
  realIndices.Sort();
}

STDMETHODIMP CAgentFolder::Extract(const UINT32 *indices, 
    UINT32 numItems, 
    NExtract::NPathMode::EEnum pathMode, 
    NExtract::NOverwriteMode::EEnum overwriteMode, 
    const wchar_t *path,
    INT32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  COM_TRY_BEGIN
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  UStringVector pathParts;
  CProxyFolder *currentProxyFolder = _proxyFolderItem;
  while (currentProxyFolder->Parent)
  {
    pathParts.Insert(0, currentProxyFolder->Name);
    currentProxyFolder = currentProxyFolder->Parent;
  }
  
  /*
  if (_flatMode)
    pathMode = NExtract::NPathMode::kNoPathnames;
  */

  extractCallbackSpec->Init(_agentSpec->GetArchive(), 
      extractCallback2, 
      false,
      path,
      pathMode, 
      overwriteMode, 
      pathParts, 
      _agentSpec->DefaultName,
      _agentSpec->DefaultTime, 
      _agentSpec->DefaultAttributes
      // ,_agentSpec->_srcDirectoryPrefix
      );
  CUIntVector realIndices;
  GetRealIndices(indices, numItems, realIndices);
  return _agentSpec->GetArchive()->Extract(&realIndices.Front(), 
      realIndices.Size(), testMode, extractCallback);
  COM_TRY_END
}

/////////////////////////////////////////
// CAgent

CAgent::CAgent():
  _proxyArchive(NULL)
{
}

CAgent::~CAgent()
{
  if (_proxyArchive != NULL)
    delete _proxyArchive;
}

STDMETHODIMP CAgent::Open(
    const wchar_t *filePath, 
    BSTR *archiveType,
    // CLSID *clsIDResult,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  _archiveFilePath = filePath;
  NFile::NFind::CFileInfoW fileInfo;
  if (!NFile::NFind::FindFile(_archiveFilePath, fileInfo))
    return ::GetLastError();
  if (fileInfo.IsDirectory())
    return E_FAIL;
  CArchiverInfo archiverInfo0, archiverInfo1;
  HRESULT res = OpenArchive(_archiveFilePath, _archiveLink, openArchiveCallback);
  // _archive = _archiveLink.GetArchive();
  DefaultName = _archiveLink.GetDefaultItemName();
  const CArchiverInfo &ai = _archiveLink.GetArchiverInfo();

  RINOK(res);
  DefaultTime = fileInfo.LastWriteTime;
  DefaultAttributes = fileInfo.Attributes;
  ArchiveType = ai.Name;
  if (archiveType != 0)
  {
    CMyComBSTR name = ArchiveType;
    *archiveType = name.Detach();
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::ReOpen(
    // const wchar_t *filePath, 
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  if (_proxyArchive != NULL)
  {
    delete _proxyArchive;
    _proxyArchive = NULL;
  }
  RINOK(ReOpenArchive(_archiveLink, _archiveFilePath));
  return ReadItems();
  COM_TRY_END
}

STDMETHODIMP CAgent::Close()
{
  COM_TRY_BEGIN
  RINOK(_archiveLink.Close());
  if (_archiveLink.GetNumLevels() > 1)
  {
    // return S_OK;
  }
  // _archive->Close();
  return S_OK;
  COM_TRY_END
}

/*
STDMETHODIMP CAgent::EnumProperties(IEnumSTATPROPSTG **EnumProperties)
{
  return _archive->EnumProperties(EnumProperties);
}
*/

HRESULT CAgent::ReadItems()
{
  if (_proxyArchive != NULL)
    return S_OK;
  _proxyArchive = new CProxyArchive();
  return _proxyArchive->Load(GetArchive(), 
      DefaultName, 
      // _defaultTime, 
      // _defaultAttributes, 
      NULL);
}

STDMETHODIMP CAgent::BindToRootFolder(IFolderFolder **resultFolder)
{
  COM_TRY_BEGIN
  RINOK(ReadItems());
  CAgentFolder *folderSpec = new CAgentFolder;
  CMyComPtr<IFolderFolder> rootFolder = folderSpec;
  folderSpec->Init(_proxyArchive, &_proxyArchive->RootFolder, NULL, this);
  *resultFolder = rootFolder.Detach();
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CAgent::Extract(
    NExtract::NPathMode::EEnum pathMode, 
    NExtract::NOverwriteMode::EEnum overwriteMode, 
    const wchar_t *path,
    INT32 testMode,
    IFolderArchiveExtractCallback *extractCallback2)
{
  COM_TRY_BEGIN
  CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
  CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
  extractCallbackSpec->Init(GetArchive(), 
      extractCallback2, 
      false,
      path,
      pathMode, 
      overwriteMode, 
      UStringVector(), 
      DefaultName,
      DefaultTime, 
      DefaultAttributes
      // ,_srcDirectoryPrefix
      );
  return GetArchive()->Extract(0, -1, testMode, extractCallback);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetNumberOfProperties(UINT32 *numProperties)
{
  COM_TRY_BEGIN
  return GetArchive()->GetNumberOfProperties(numProperties);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetPropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  RINOK(GetArchive()->GetPropertyInfo(index, name, propID, varType));
  if (*propID == kpidPath)
    *propID = kpidName;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  return GetArchive()->GetArchiveProperty(propID, value);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  COM_TRY_BEGIN
  return GetArchive()->GetNumberOfArchiveProperties(numProperties);
  COM_TRY_END
}

STDMETHODIMP CAgent::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  COM_TRY_BEGIN
  return GetArchive()->GetArchivePropertyInfo(index,     
      name, propID, varType);
  COM_TRY_END
}

