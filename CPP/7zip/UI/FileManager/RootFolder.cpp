// RootFolder.cpp

#include "StdAfx.h"

#include "resource.h"

#include "RootFolder.h"

#include "Common/StringConvert.h"
#include "../../PropID.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "FSDrives.h"
#include "PhysDriveFolder.h"
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

UString RootFolder_GetName_Computer(int &iconIndex)
{
  iconIndex = GetIconIndexForCSIDL(CSIDL_DRIVES);
  return LangString(IDS_COMPUTER, 0x03020300);
}

UString RootFolder_GetName_Network(int &iconIndex)
{
  iconIndex = GetIconIndexForCSIDL(CSIDL_NETWORK);
  return LangString(IDS_NETWORK, 0x03020301);
}

UString RootFolder_GetName_Documents(int &iconIndex)
{
  iconIndex = GetIconIndexForCSIDL(CSIDL_PERSONAL);
  return LangString(IDS_DOCUMENTS, 0x03020302); ;
}

const int ROOT_INDEX_COMPUTER = 0;
const int ROOT_INDEX_DOCUMENTS = 1;
const int ROOT_INDEX_NETWORK = 2;

void CRootFolder::Init()
{
  _names[ROOT_INDEX_COMPUTER] = RootFolder_GetName_Computer(_iconIndices[ROOT_INDEX_COMPUTER]);
  _names[ROOT_INDEX_DOCUMENTS] = RootFolder_GetName_Documents(_iconIndices[ROOT_INDEX_DOCUMENTS]);
  _names[ROOT_INDEX_NETWORK] = RootFolder_GetName_Network(_iconIndices[ROOT_INDEX_NETWORK]);
};

STDMETHODIMP CRootFolder::LoadItems()
{
  Init();
  return S_OK;
}

STDMETHODIMP CRootFolder::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = kNumRootFolderItems;
  return S_OK;
}

STDMETHODIMP CRootFolder::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidIsDir:  prop = true; break;
    case kpidName:  prop = _names[itemIndex]; break;
  }
  prop.Detach(value);
  return S_OK;
}

UString GetMyDocsPath()
{
  UString us;
  WCHAR s[MAX_PATH + 1];
  if (SHGetSpecialFolderPathW(0, s, CSIDL_PERSONAL, FALSE))
    us = s;
  #ifndef _UNICODE
  else
  {
    CHAR s2[MAX_PATH + 1];
    if (SHGetSpecialFolderPathA(0, s2, CSIDL_PERSONAL, FALSE))
      us = GetUnicodeString(s2);
  }
  #endif
  if (us.Length() > 0 && us[us.Length() - 1] != WCHAR_PATH_SEPARATOR)
    us += WCHAR_PATH_SEPARATOR;
  return us;
}

STDMETHODIMP CRootFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  if (index == ROOT_INDEX_COMPUTER)
  {
    CFSDrives *fsDrivesSpec = new CFSDrives;
    CMyComPtr<IFolderFolder> subFolder = fsDrivesSpec;
    fsDrivesSpec->Init();
    *resultFolder = subFolder.Detach();
  }
  else if (index == ROOT_INDEX_NETWORK)
  {
    CNetFolder *netFolderSpec = new CNetFolder;
    CMyComPtr<IFolderFolder> subFolder = netFolderSpec;
    netFolderSpec->Init(0, 0, _names[ROOT_INDEX_NETWORK] + WCHAR_PATH_SEPARATOR);
    *resultFolder = subFolder.Detach();
  }
  else if (index == ROOT_INDEX_DOCUMENTS)
  {
    UString s = GetMyDocsPath();
    if (!s.IsEmpty())
    {
      NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
      CMyComPtr<IFolderFolder> subFolder = fsFolderSpec;
      RINOK(fsFolderSpec->Init(s, NULL));
      *resultFolder = subFolder.Detach();
    }
  }
  else
    return E_INVALIDARG;
  return S_OK;
}

static bool AreEqualNames(const UString &name1, const UString &name2)
{
  return (name1 == name2 || name1 == (name2 + UString(WCHAR_PATH_SEPARATOR)));
}

STDMETHODIMP CRootFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  UString name2 = name;
  name2.Trim();
  if (name2.IsEmpty())
  {
    CRootFolder *rootFolderSpec = new CRootFolder;
    CMyComPtr<IFolderFolder> rootFolder = rootFolderSpec;
    rootFolderSpec->Init();
    *resultFolder = rootFolder.Detach();
    return S_OK;
  }
  for (int i = 0; i < kNumRootFolderItems; i++)
    if (AreEqualNames(name2, _names[i]))
      return BindToFolder((UInt32)i, resultFolder);
  if (AreEqualNames(name2, L"My Documents") ||
      AreEqualNames(name2, L"Documents"))
    return BindToFolder((UInt32)ROOT_INDEX_DOCUMENTS, resultFolder);
  if (AreEqualNames(name2, L"My Computer") ||
      AreEqualNames(name2, L"Computer"))
    return BindToFolder((UInt32)ROOT_INDEX_COMPUTER, resultFolder);
  if (name2 == UString(WCHAR_PATH_SEPARATOR))
  {
    CMyComPtr<IFolderFolder> subFolder = this;
    *resultFolder = subFolder.Detach();
    return S_OK;
  }

  if (name2.Length () < 2)
    return E_INVALIDARG;

  CMyComPtr<IFolderFolder> subFolder;
  
  if (name2.Left(4) == L"\\\\.\\")
  {
    CPhysDriveFolder *folderSpec = new CPhysDriveFolder;
    subFolder = folderSpec;
    RINOK(folderSpec->Init(name2.Mid(4, 2)));
  }
  else
  {
    if (name2[name2.Length () - 1] != WCHAR_PATH_SEPARATOR)
      name2 += WCHAR_PATH_SEPARATOR;
    NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
    subFolder = fsFolderSpec;
    if (fsFolderSpec->Init(name2, 0) != S_OK)
    {
      if (name2[0] == WCHAR_PATH_SEPARATOR)
      {
        CNetFolder *netFolderSpec = new CNetFolder;
        subFolder = netFolderSpec;
        netFolderSpec->Init(name2);
      }
      else
        return E_INVALIDARG;
    }
  }
  *resultFolder = subFolder.Detach();
  return S_OK;
}

STDMETHODIMP CRootFolder::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  return S_OK;
}

STDMETHODIMP CRootFolder::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CRootFolder::GetPropertyInfo(UInt32 index,
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

STDMETHODIMP CRootFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidType: prop = L"RootFolder"; break;
    case kpidPath: prop = L""; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CRootFolder::GetSystemIconIndex(UInt32 index, INT32 *iconIndex)
{
  *iconIndex = _iconIndices[index];
  return S_OK;
}
