// FSDrives.cpp

#include "StdAfx.h"

#include "resource.h"

#include "FSDrives.h"

#include "Common/StringConvert.h"
#include "../PropID.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileSystem.h"

#include "SysIconUtils.h"
#include "FSFolder.h"
#include "LangUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

static const STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  { L"Total Size", kpidTotalSize, VT_UI8},
  { L"Free Space", kpidFreeSpace, VT_UI8},
  { NULL, kpidType, VT_BSTR},
  { L"Label", kpidVolumeName, VT_BSTR},
  { L"File system", kpidFileSystem, VT_BSTR},
  { L"Cluster Size", kpidClusterSize, VT_UI8} 
};

static const wchar_t *kDriveTypes[] =
{
  L"Unknown",
  L"No Root Dir",
  L"Removable",
  L"Fixed",
  L"Remote",
  L"CD-ROM",
  L"RAM disk"
};

static inline UINT GetCurrentFileCodePage()
  { return AreFileApisANSI() ? CP_ACP : CP_OEMCP; }

STDMETHODIMP CFSDrives::LoadItems()
{
  UINT fileCodePage = GetCurrentFileCodePage();
  _drives.Clear();

  CSysStringVector driveStrings;
  MyGetLogicalDriveStrings(driveStrings);
  for (int i = 0; i < driveStrings.Size(); i++)
  {
    CDriveInfo driveInfo;

    const CSysString &driveName = driveStrings[i];

    driveInfo.FullSystemName = GetUnicodeString(driveName, fileCodePage);

    driveInfo.Name = driveInfo.FullSystemName.Left(
        driveInfo.FullSystemName.Length() - 1);
    driveInfo.ClusterSize = 0;
    driveInfo.DriveSize = 0;
    driveInfo.FreeSpace = 0;
    UINT driveType = ::GetDriveType(driveName);
    if (driveType < sizeof(kDriveTypes) / sizeof(kDriveTypes[0]))
    {
      driveInfo.Type = kDriveTypes[driveType];
    }
    bool needRead = true;
    if (driveType == DRIVE_CDROM || driveType == DRIVE_REMOVABLE)
    {
      /*
      DWORD dwSerialNumber;`
      if (!::GetVolumeInformation(driveInfo.FullSystemName, 
          NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0)) 
      */
      driveInfo.KnownSizes = false;
      {
        needRead = false;
      }
    }
    if (needRead)
    {
      CSysString volumeName, fileSystemName;
      DWORD volumeSerialNumber, maximumComponentLength, fileSystemFlags;
      NFile::NSystem::MyGetVolumeInformation(driveName, 
          volumeName,
          &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags, 
          fileSystemName);
      driveInfo.VolumeName = GetUnicodeString(volumeName, fileCodePage);
      driveInfo.FileSystemName = GetUnicodeString(fileSystemName, fileCodePage);

      NFile::NSystem::MyGetDiskFreeSpace(driveName,
          driveInfo.ClusterSize, driveInfo.DriveSize, driveInfo.FreeSpace);
      driveInfo.KnownSizes = true;
    }
    _drives.Add(driveInfo);
  }
  return S_OK;
}

STDMETHODIMP CFSDrives::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _drives.Size();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetProperty(UINT32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  if (itemIndex >= (UINT32)_drives.Size())
    return E_INVALIDARG;
  NCOM::CPropVariant propVariant;
  const CDriveInfo &driveInfo = _drives[itemIndex];
  switch(propID)
  {
    case kpidIsFolder:
      propVariant = true;
      break;
    case kpidName:
      propVariant = driveInfo.Name;
      break;
    case kpidTotalSize:
      if (driveInfo.KnownSizes)
        propVariant = driveInfo.DriveSize;
      break;
    case kpidFreeSpace:
      if (driveInfo.KnownSizes)
        propVariant = driveInfo.FreeSpace;
      break;
    case kpidClusterSize:
      if (driveInfo.KnownSizes)
        propVariant = driveInfo.ClusterSize;
      break;
    case kpidType:
      propVariant = driveInfo.Type;
      break;
    case kpidVolumeName:
      propVariant = driveInfo.VolumeName;
      break;
    case kpidFileSystem:
      propVariant = driveInfo.FileSystemName;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}

HRESULT CFSDrives::BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  CFSFolder *fsFolderSpec = new CFSFolder;
  CMyComPtr<IFolderFolder> subFolder = fsFolderSpec;
  RINOK(fsFolderSpec->Init(name, 0));
  *resultFolder = subFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (index >= (UINT32)_drives.Size())
    return E_INVALIDARG;
  return BindToFolderSpec(_drives[index].FullSystemName, resultFolder);
}

STDMETHODIMP CFSDrives::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  return BindToFolderSpec(name, resultFolder);
}

STDMETHODIMP CFSDrives::BindToParentFolder(IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  return S_OK;
}

STDMETHODIMP CFSDrives::GetName(BSTR *name)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CFSDrives::GetPropertyInfo(UINT32 index,     
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


STDMETHODIMP CFSDrives::GetTypeID(BSTR *name)
{
  CMyComBSTR temp = L"FSDrives";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetPath(BSTR *path)
{
  CMyComBSTR temp = (LangLoadStringW(IDS_COMPUTER, 0x03020300)) + 
      UString(L'\\');
  *path = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  *iconIndex = 0;
  const CDriveInfo &driveInfo = _drives[index];
  int iconIndexTemp;
  if (GetRealIconIndex(driveInfo.FullSystemName, 0, iconIndexTemp) != 0)
  {
    *iconIndex = iconIndexTemp;
    return S_OK;
  }
  return GetLastError();
}

