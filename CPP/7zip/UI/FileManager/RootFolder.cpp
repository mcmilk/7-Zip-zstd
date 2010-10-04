// RootFolder.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/PropVariant.h"

#include "../../PropID.h"

#include "FSFolder.h"
#include "LangUtils.h"
#ifndef UNDER_CE
#include "NetFolder.h"
#include "FSDrives.h"
#endif
#include "RootFolder.h"
#include "SysIconUtils.h"

#include "resource.h"

using namespace NWindows;

static const STATPROPSTG kProps[] =
{
  { NULL, kpidName, VT_BSTR}
};

UString RootFolder_GetName_Computer(int &iconIndex)
{
  #ifdef UNDER_CE
  GetRealIconIndex(L"\\", FILE_ATTRIBUTE_DIRECTORY, iconIndex);
  #else
  iconIndex = GetIconIndexForCSIDL(CSIDL_DRIVES);
  #endif
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

enum
{
  ROOT_INDEX_COMPUTER = 0
  #ifndef UNDER_CE
  , ROOT_INDEX_DOCUMENTS
  , ROOT_INDEX_NETWORK
  , ROOT_INDEX_VOLUMES
  #endif
};

#ifndef UNDER_CE
static const wchar_t *kVolPrefix = L"\\\\.";
#endif

void CRootFolder::Init()
{
  _names[ROOT_INDEX_COMPUTER] = RootFolder_GetName_Computer(_iconIndices[ROOT_INDEX_COMPUTER]);
  #ifndef UNDER_CE
  _names[ROOT_INDEX_DOCUMENTS] = RootFolder_GetName_Documents(_iconIndices[ROOT_INDEX_DOCUMENTS]);
  _names[ROOT_INDEX_NETWORK] = RootFolder_GetName_Network(_iconIndices[ROOT_INDEX_NETWORK]);
  _names[ROOT_INDEX_VOLUMES] = kVolPrefix;
  _iconIndices[ROOT_INDEX_VOLUMES] = GetIconIndexForCSIDL(CSIDL_DRIVES);
  #endif
}

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

typedef BOOL (WINAPI *SHGetSpecialFolderPathWp)(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate);
typedef BOOL (WINAPI *SHGetSpecialFolderPathAp)(HWND hwnd, LPSTR pszPath, int csidl, BOOL fCreate);

UString GetMyDocsPath()
{
  UString us;
  WCHAR s[MAX_PATH + 1];
  SHGetSpecialFolderPathWp getW = (SHGetSpecialFolderPathWp)
      #ifdef UNDER_CE
      My_GetProcAddress(GetModuleHandle(TEXT("coredll.dll")), "SHGetSpecialFolderPath");
      #else
      My_GetProcAddress(GetModuleHandle(TEXT("shell32.dll")), "SHGetSpecialFolderPathW");
      #endif
  if (getW && getW(0, s, CSIDL_PERSONAL, FALSE))
    us = s;
  #ifndef _UNICODE
  else
  {
    SHGetSpecialFolderPathAp getA = (SHGetSpecialFolderPathAp)
        ::GetProcAddress(::GetModuleHandleA("shell32.dll"), "SHGetSpecialFolderPathA");
    CHAR s2[MAX_PATH + 1];
    if (getA && getA(0, s2, CSIDL_PERSONAL, FALSE))
      us = GetUnicodeString(s2);
  }
  #endif
  if (us.Length() > 0 && us[us.Length() - 1] != WCHAR_PATH_SEPARATOR)
    us += WCHAR_PATH_SEPARATOR;
  return us;
}

STDMETHODIMP CRootFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  *resultFolder = NULL;
  CMyComPtr<IFolderFolder> subFolder;
  #ifdef UNDER_CE
  if (index == ROOT_INDEX_COMPUTER)
  {
    NFsFolder::CFSFolder *fsFolder = new NFsFolder::CFSFolder;
    subFolder = fsFolder;
    fsFolder->InitToRoot();
  }
  #else
  if (index == ROOT_INDEX_COMPUTER || index == ROOT_INDEX_VOLUMES)
  {
    CFSDrives *fsDrivesSpec = new CFSDrives;
    subFolder = fsDrivesSpec;
    fsDrivesSpec->Init(index == ROOT_INDEX_VOLUMES);
  }
  else if (index == ROOT_INDEX_NETWORK)
  {
    CNetFolder *netFolderSpec = new CNetFolder;
    subFolder = netFolderSpec;
    netFolderSpec->Init(0, 0, _names[ROOT_INDEX_NETWORK] + WCHAR_PATH_SEPARATOR);
  }
  else if (index == ROOT_INDEX_DOCUMENTS)
  {
    UString s = GetMyDocsPath();
    if (!s.IsEmpty())
    {
      NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
      subFolder = fsFolderSpec;
      RINOK(fsFolderSpec->Init(s, NULL));
    }
  }
  #endif
  else
    return E_INVALIDARG;
  *resultFolder = subFolder.Detach();
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
  #ifdef UNDER_CE
  if (name2 == L"\\")
    return BindToFolder((UInt32)ROOT_INDEX_COMPUTER, resultFolder);
  #else
  if (AreEqualNames(name2, L"My Documents") ||
      AreEqualNames(name2, L"Documents"))
    return BindToFolder((UInt32)ROOT_INDEX_DOCUMENTS, resultFolder);
  #endif
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
  
  #ifndef UNDER_CE
  if (name2.Left(4) == kVolPrefix)
  {
    CFSDrives *folderSpec = new CFSDrives;
    subFolder = folderSpec;
    folderSpec->Init(true);
  }
  else
  #endif
  {
    if (name2[name2.Length () - 1] != WCHAR_PATH_SEPARATOR)
      name2 += WCHAR_PATH_SEPARATOR;
    NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
    subFolder = fsFolderSpec;
    if (fsFolderSpec->Init(name2, 0) != S_OK)
    {
      #ifndef UNDER_CE
      if (name2[0] == WCHAR_PATH_SEPARATOR)
      {
        CNetFolder *netFolderSpec = new CNetFolder;
        subFolder = netFolderSpec;
        netFolderSpec->Init(name2);
      }
      else
      #endif
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

IMP_IFolderFolder_Props(CRootFolder)

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

STDMETHODIMP CRootFolder::GetSystemIconIndex(UInt32 index, Int32 *iconIndex)
{
  *iconIndex = _iconIndices[index];
  return S_OK;
}
