// RootFolder.cpp

#include "StdAfx.h"

#include "resource.h"

#include "RootFolder.h"

#include "Common/StringConvert.h"
#include "Interface/PropID.h"
#include "Interface/EnumStatProp.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "FSDrives.h"
#include "NetFolder.h"
#include "SysIconUtils.h"
#include "LangUtils.h"

using namespace NWindows;


static const STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR}
};

// static const wchar_t *kMyComputerTitle = L"Computer";
// static const wchar_t *kMyNetworkTitle = L"Network";

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

void CRootFolder::Init() 
{
  _computerName = LangLoadStringW(IDS_COMPUTER, 0x03020300);
  _networkName = LangLoadStringW(IDS_NETWORK, 0x03020301);
};

STDMETHODIMP CRootFolder::LoadItems()
{
  Init();
  return S_OK;
}

STDMETHODIMP CRootFolder::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = 2;
  return S_OK;
}

STDMETHODIMP CRootFolder::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = true;
      break;
    case kpidName:
      if (itemIndex == 0)
        propVariant = _computerName;
      else
        propVariant = _networkName;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

STDMETHODIMP CRootFolder::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  if (index == 0)
  {
    CComObjectNoLock<CFSDrives> *fsDrivesSpec = new CComObjectNoLock<CFSDrives>;
    CComPtr<IFolderFolder> subFolder = fsDrivesSpec;
    fsDrivesSpec->Init();
    *resultFolder = subFolder.Detach();
  }
  else
  {
    CComObjectNoLock<CNetFolder> *netFolderSpec = new CComObjectNoLock<CNetFolder>;
    CComPtr<IFolderFolder> subFolder = netFolderSpec;
    netFolderSpec->Init(0, 0, _networkName + L'\\');
    *resultFolder = subFolder.Detach();
  }
  return S_OK;
}

STDMETHODIMP CRootFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  UString name2 = name;
  name2.Trim();
  if (name2.IsEmpty())
  {
    CComObjectNoLock<CRootFolder> *rootFolderSpec = new CComObjectNoLock<CRootFolder>;
    CComPtr<IFolderFolder> rootFolder = rootFolderSpec;
    rootFolderSpec->Init();
    *resultFolder = rootFolder.Detach();
    return S_OK;
  }
  if (name2 == _computerName || 
      name2 == (_computerName + UString(L'\\')))
    return BindToFolder(UINT32(0), resultFolder);
  if (name2 == _networkName || 
      name2 == (_networkName + UString(L'\\')))
    return BindToFolder(UINT32(1), resultFolder);
  if (name2 == UString(L'\\'))
  {
    CComPtr<IFolderFolder> subFolder = this;
    *resultFolder = subFolder.Detach();
    return S_OK;
  }

  if (name2.Length () < 2)
    return E_INVALIDARG;

  if (name2[name2.Length () - 1] != L'\\')
    name2 += L'\\';
  CComObjectNoLock<CFSFolder> *fsFolderSpec = new CComObjectNoLock<CFSFolder>;
  CComPtr<IFolderFolder> subFolder = fsFolderSpec;
  if (fsFolderSpec->Init(GetSystemString(name2, GetCurrentFileCodePage()), 0) == S_OK)
  {
    *resultFolder = subFolder.Detach();
    return S_OK;
  }
  if (name2[0] == L'\\')
  {
    CComObjectNoLock<CNetFolder> *netFolderSpec = new CComObjectNoLock<CNetFolder>;
    CComPtr<IFolderFolder> subFolder = netFolderSpec;
    netFolderSpec->Init(name2);
    *resultFolder = subFolder.Detach();
    return S_OK;
  }
  return E_INVALIDARG;
}

STDMETHODIMP CRootFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  return S_OK;
}

STDMETHODIMP CRootFolder::GetName(BSTR *name)
{
  return E_NOTIMPL;
}


STDMETHODIMP CRootFolder::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}

STDMETHODIMP CRootFolder::GetTypeID(BSTR *name)
{
  CComBSTR temp = L"RootFolder";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CRootFolder::GetPath(BSTR *path)
{
  CComBSTR temp = L"";
  *path = temp.Detach();
  return S_OK;
}

STDMETHODIMP CRootFolder::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  int aCSIDL;
  if (index == 0)
    aCSIDL = CSIDL_DRIVES;
  else
    aCSIDL = CSIDL_NETWORK;
  *iconIndex = GetIconIndexForCSIDL(aCSIDL);
  return S_OK;
}




