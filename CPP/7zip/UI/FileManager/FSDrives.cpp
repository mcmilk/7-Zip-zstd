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
  // { NULL, kpidIsDir, VT_BOOL},
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
    CDriveInfo di;

    const UString &driveName = driveStrings[i];

    di.FullSystemName = driveName;

    di.Name = di.FullSystemName.Left(
        di.FullSystemName.Length() - 1);
    di.ClusterSize = 0;
    di.DriveSize = 0;
    di.FreeSpace = 0;
    UINT driveType = NFile::NSystem::MyGetDriveType(driveName);
    if (driveType < sizeof(kDriveTypes) / sizeof(kDriveTypes[0]))
    {
      di.Type = kDriveTypes[driveType];
    }
    bool needRead = true;
    if (driveType == DRIVE_CDROM || driveType == DRIVE_REMOVABLE)
    {
      /*
      DWORD dwSerialNumber;`
      if (!::GetVolumeInformation(di.FullSystemName,
          NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0))
      */
      di.KnownSizes = false;
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
      di.VolumeName = volumeName;
      di.FileSystemName = fileSystemName;

      NFile::NSystem::MyGetDiskFreeSpace(driveName,
          di.ClusterSize, di.DriveSize, di.FreeSpace);
      di.KnownSizes = true;
    }
    _drives.Add(di);
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
  NCOM::CPropVariant prop;
  const CDriveInfo &di = _drives[itemIndex];
  switch(propID)
  {
    case kpidIsDir:  prop = true; break;
    case kpidName:  prop = di.Name; break;
    case kpidTotalSize:   if (di.KnownSizes) prop = di.DriveSize; break;
    case kpidFreeSpace:   if (di.KnownSizes) prop = di.FreeSpace; break;
    case kpidClusterSize: if (di.KnownSizes) prop = di.ClusterSize; break;
    case kpidType:  prop = di.Type; break;
    case kpidVolumeName:  prop = di.VolumeName; break;
    case kpidFileSystem:  prop = di.FileSystemName; break;
  }
  prop.Detach(value);
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
  const CDriveInfo &di = _drives[index];
  if (_volumeMode)
  {
    *resultFolder = 0;
    CPhysDriveFolder *folderSpec = new CPhysDriveFolder;
    CMyComPtr<IFolderFolder> subFolder = folderSpec;
    RINOK(folderSpec->Init(di.Name));
    *resultFolder = subFolder.Detach();
    return S_OK;
  }
  return BindToFolderSpec(di.FullSystemName, resultFolder);
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
  const CDriveInfo &di = _drives[index];
  int iconIndexTemp;
  if (GetRealIconIndex(di.FullSystemName, 0, iconIndexTemp) != 0)
  {
    *iconIndex = iconIndexTemp;
    return S_OK;
  }
  return GetLastError();
}

