// NetFolder.cpp

#include "StdAfx.h"

#include "NetFolder.h"

#include "Common/StringConvert.h"
#include "../PropID.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "SysIconUtils.h"
#include "FSFolder.h"

using namespace NWindows;
using namespace NNet;

static const STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidLocalName, VT_BSTR},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidProvider, VT_BSTR}
};

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CNetFolder::Init(const UString &path)
{
  /*
  if (path.Length() > 2)
  {
    if (path[0] == L'\\' && path[1] == L'\\')
    {
      CResource netResource;
      netResource.RemoteName = GetSystemString(path.Left(path.Length() - 1));
      netResource.Scope = RESOURCE_GLOBALNET;
      netResource.Type = RESOURCETYPE_DISK;
      netResource.DisplayType = RESOURCEDISPLAYTYPE_SERVER;
      netResource.Usage = RESOURCEUSAGE_CONTAINER;
      Init(&netResource, 0, path);
      return;
    }
  }
  Init(0, 0 , L"");
  */
  CResource resource;
  resource.RemoteNameIsDefined = true;
  resource.RemoteName = GetSystemString(path.Left(path.Length() - 1));
  resource.ProviderIsDefined = false;
  resource.LocalNameIsDefined = false;
  resource.CommentIsDefined = false;
  resource.Type = RESOURCETYPE_DISK;
  resource.Scope = RESOURCE_GLOBALNET;
  resource.Usage = 0;
  resource.DisplayType = 0;
  CResource aDestResource;
  CSysString aSystemPathPart;
  DWORD result = GetResourceInformation(resource, aDestResource, 
      aSystemPathPart);
  if (result == NO_ERROR)
    Init(&aDestResource, 0, path);
  else
    Init(0, 0 , L"");
  return;
}

void CNetFolder::Init(const NWindows::NNet::CResource *netResource, 
      IFolderFolder *parentFolder, const UString &path)
{
  _path = path;
  if (netResource == 0)
    _netResourcePointer = 0;
  else
  {
    _netResource = *netResource;
    _netResourcePointer = &_netResource;

    // if (_netResource.DisplayType == RESOURCEDISPLAYTYPE_SERVER)
      _path = GetUnicodeString(_netResource.RemoteName) + L'\\';
  }
  _parentFolder = parentFolder;
}

STDMETHODIMP CNetFolder::LoadItems()
{
  _items.Clear();
  CEnum enumerator;

  while(true)
  {
    DWORD result = enumerator.Open(
      RESOURCE_GLOBALNET,
      RESOURCETYPE_DISK,
      0,        // enumerate all resources
      _netResourcePointer
      );
    if (result == NO_ERROR)
      break;
    if (result != ERROR_ACCESS_DENIED)
      return result;
    if (_netResourcePointer != 0)
    result = AddConnection2(_netResource,
        0, 0, CONNECT_INTERACTIVE);
    if (result != NO_ERROR)
      return result;
  }

  while(true)
  {  
    CResourceEx resource;
    DWORD result = enumerator.Next(resource);
    if (result == NO_ERROR)
    {
      if (!resource.RemoteNameIsDefined) // For Win 98, I don't know what's wrong
        resource.RemoteName = resource.Comment;
      resource.Name = GetUnicodeString(resource.RemoteName);
      int aPos = resource.Name.ReverseFind(L'\\');
      if (aPos >= 0)
      {
        // _path = resource.Name.Left(aPos + 1);
        resource.Name = resource.Name.Mid(aPos + 1);
      }
      _items.Add(resource);
    }
    else if (result == ERROR_NO_MORE_ITEMS)
      break;
    else 
      return result;
  }

  if (_netResourcePointer && _netResource.DisplayType == RESOURCEDISPLAYTYPE_SERVER)
  {
    for (char c = 'a'; c <= 'z'; c++)
    {
      CResourceEx resource;
      resource.Name = UString(wchar_t(c)) + L'$';
      resource.RemoteNameIsDefined = true;
      resource.RemoteName = GetSystemString(_path + resource.Name);

      NFile::NFind::CFindFile aFindFile;
      NFile::NFind::CFileInfo aFileInfo;
      if (!aFindFile.FindFirst(resource.RemoteName + CSysString(TEXT("\\*")), aFileInfo))
        continue;
      resource.Usage = RESOURCEUSAGE_CONNECTABLE;
      resource.LocalNameIsDefined = false;
      resource.CommentIsDefined = false;
      resource.ProviderIsDefined = false;
      _items.Add(resource);
    }
  }
  return S_OK;
}


STDMETHODIMP CNetFolder::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CNetFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  const CResourceEx &item = _items[itemIndex];
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = true;
      break;
    case kpidName:
      // if (item.RemoteNameIsDefined)
        propVariant = item.Name;
      break;
    case kpidLocalName:
      if (item.LocalNameIsDefined)
        propVariant = GetUnicodeString(item.LocalName);
      break;
    case kpidComment:
      if (item.CommentIsDefined)
        propVariant = GetUnicodeString(item.Comment);
      break;
    case kpidProvider:
      if (item.ProviderIsDefined)
        propVariant = GetUnicodeString(item.Provider);
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CNetFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  const CResourceEx &resource = _items[index];
  if (resource.Usage == RESOURCEUSAGE_CONNECTABLE || 
      resource.DisplayType == RESOURCEDISPLAYTYPE_SHARE)
  {
    CFSFolder *fsFolderSpec = new CFSFolder;
    CMyComPtr<IFolderFolder> subFolder = fsFolderSpec;
    RINOK(fsFolderSpec->Init(resource.RemoteName + TEXT('\\'), this));
    *resultFolder = subFolder.Detach();
  }
  else
  {
    CNetFolder *netFolder = new CNetFolder;
    CMyComPtr<IFolderFolder> subFolder = netFolder;
    netFolder->Init(&resource, this, GetUnicodeString(resource.Name) + L'\\');
    *resultFolder = subFolder.Detach();
  }
  return S_OK;
}

STDMETHODIMP CNetFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  return E_NOTIMPL;
}

STDMETHODIMP CNetFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (_parentFolder)
  {
    CMyComPtr<IFolderFolder> parentFolder = _parentFolder;
    *resultFolder = parentFolder.Detach();
    return S_OK;
  }
  if (_netResourcePointer != 0)
  {
    CResource resourceParent;
    DWORD result = GetResourceParent(_netResource, resourceParent);
    if (result != NO_ERROR)
      return result;
    if (!_netResource.RemoteNameIsDefined)
      return S_OK;

    CNetFolder *netFolder = new CNetFolder;
    CMyComPtr<IFolderFolder> subFolder = netFolder;
    netFolder->Init(&resourceParent, 0, L'\\');
    *resultFolder = subFolder.Detach();
  }
  return S_OK;
}

STDMETHODIMP CNetFolder::GetName(BSTR *name)
{
  *name = 0;
  return E_NOTIMPL;
  /*
  CMyComBSTR aBSTRName = m_ProxyFolderItem->m_Name;
  *aName = aBSTRName.Detach();
  return S_OK;
  */
}

STDMETHODIMP CNetFolder::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CNetFolder::GetPropertyInfo(UINT32 index,     
    BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if (index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &prop = kProperties[index];
  *propID = prop.propid;
  *varType = prop.vt;
  *name = 0;
  return S_OK;
}

STDMETHODIMP CNetFolder::GetTypeID(BSTR *name)
{
  CMyComBSTR aBSTRName = L"NetFolder";
  *name = aBSTRName.Detach();
  return S_OK;
}

STDMETHODIMP CNetFolder::GetPath(BSTR *path)
{
  CMyComBSTR aBSTRName = _path;
  *path = aBSTRName.Detach();
  return S_OK;
}

STDMETHODIMP CNetFolder::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  const CResource &resource = _items[index];
  if (resource.DisplayType == RESOURCEDISPLAYTYPE_SERVER || 
      resource.Usage == RESOURCEUSAGE_CONNECTABLE)
    *iconIndex = GetRealIconIndex(0, resource.RemoteName);
  else
  {
    *iconIndex = GetRealIconIndex(FILE_ATTRIBUTE_DIRECTORY, TEXT(""));
    // *anIconIndex = GetRealIconIndex(0, L"\\\\HOME");
  }
  return S_OK;
}
