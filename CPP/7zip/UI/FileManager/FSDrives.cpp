// FSDrives.cpp

#include "StdAfx.h"

#include "resource.h"

#include "FSDrives.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "../../PropID.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"
#include "Windows/FileSystem.h"

#include "SysIconUtils.h"
#include "FSFolder.h"
#include "PhysDriveFolder.h"
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

STDMETHODIMP CFSDrives::LoadItems()
{
  _drives.Clear();

  UStringVector driveStrings;
  MyGetLogicalDriveStrings(driveStrings);
  for (int i = 0; i < driveStrings.Size(); i++)
  {
    CDriveInfo driveInfo;

    const UString &driveName = driveStrings[i];

    driveInfo.FullSystemName = driveName;

    driveInfo.Name = driveInfo.FullSystemName.Left(
        driveInfo.FullSystemName.Length() - 1);
    driveInfo.ClusterSize = 0;
    driveInfo.DriveSize = 0;
    driveInfo.FreeSpace = 0;
    UINT driveType = NFile::NSystem::MyGetDriveType(driveName);
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
      UString volumeName, fileSystemName;
      DWORD volumeSerialNumber, maximumComponentLength, fileSystemFlags;
      NFile::NSystem::MyGetVolumeInformation(driveName, 
          volumeName,
          &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags, 
          fileSystemName);
      driveInfo.VolumeName = volumeName;
      driveInfo.FileSystemName = fileSystemName;

      NFile::NSystem::MyGetDiskFreeSpace(driveName,
          driveInfo.ClusterSize, driveInfo.DriveSize, driveInfo.FreeSpace);
      driveInfo.KnownSizes = true;
    }
    _drives.Add(driveInfo);
  }
  return S_OK;
}

STDMETHODIMP CFSDrives::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _drives.Size();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)
{
  if (itemIndex >= (UInt32)_drives.Size())
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
  NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
  CMyComPtr<IFolderFolder> subFolder = fsFolderSpec;
  RINOK(fsFolderSpec->Init(name, 0));
  *resultFolder = subFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::BindToFolder(UInt32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (index >= (UInt32)_drives.Size())
    return E_INVALIDARG;
  const CDriveInfo &driveInfo = _drives[index];
  if (_volumeMode)
  {
    *resultFolder = 0;
    CPhysDriveFolder *folderSpec = new CPhysDriveFolder;
    CMyComPtr<IFolderFolder> subFolder = folderSpec;
    RINOK(folderSpec->Init(driveInfo.Name));
    *resultFolder = subFolder.Detach();
    return S_OK;
  }
  return BindToFolderSpec(driveInfo.FullSystemName, resultFolder);
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

STDMETHODIMP CFSDrives::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CFSDrives::GetPropertyInfo(UInt32 index,     
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


STDMETHODIMP CFSDrives::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidType: prop = L"FSDrives"; break;
    case kpidPath: prop = LangString(IDS_COMPUTER, 0x03020300) +  UString(L'\\'); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CFSDrives::GetSystemIconIndex(UInt32 index, INT32 *iconIndex)
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

