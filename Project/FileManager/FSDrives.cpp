// FSDrives.cpp

#include "StdAfx.h"

#include "resource.h"

#include "FSDrives.h"

#include "Common/StringConvert.h"
#include "Interface/PropID.h"
#include "Interface/EnumStatProp.h"
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

enum // PropID
{
  /*
  kpidTotalSize = kpidUserDefined,
  kpidFreeSpace, 
  kpidClusterSize,
  kpidVolumeName,
  kpidFileSystemName
  */
};

static const STATPROPSTG kProperties[] = 
{
  { NULL, kpidName, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  { L"Total Size", kpidTotalSize, VT_UI8},
  { L"Free Space", kpidFreeSpace, VT_UI8},
  { L"Cluster Size", kpidClusterSize, VT_UI8},
  { NULL, kpidType, VT_BSTR},
  { L"Label", kpidVolumeName, VT_BSTR},
  { L"File system", kpidFileSystem, VT_BSTR}
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

    driveInfo.FullSystemName = driveStrings[i];

    driveInfo.Name = GetUnicodeString(
        driveInfo.FullSystemName.Left(driveInfo.FullSystemName.Length() - 1),
        fileCodePage);
    driveInfo.ClusterSize = 0;
    driveInfo.DriveSize = 0;
    driveInfo.FreeSpace = 0;
    UINT aDriveType = ::GetDriveType(driveInfo.FullSystemName);
    if (aDriveType < sizeof(kDriveTypes) / sizeof(kDriveTypes[0]))
    {
      driveInfo.Type = kDriveTypes[aDriveType];
    }
    bool needRead = true;
    if (aDriveType == DRIVE_CDROM || aDriveType == DRIVE_REMOVABLE)
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
      NFile::NSystem::MyGetVolumeInformation(driveInfo.FullSystemName, 
          volumeName,
          &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags, 
          fileSystemName);
      driveInfo.VolumeName = GetUnicodeString(volumeName, fileCodePage);
      driveInfo.FileSystemName = GetUnicodeString(fileSystemName, fileCodePage);

      NFile::NSystem::MyGetDiskFreeSpace(driveInfo.FullSystemName,
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
  if (itemIndex >= _drives.Size())
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

HRESULT CFSDrives::BindToFolderSpec(const TCHAR *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  CComObjectNoLock<CFSFolder> *fsFolderSpec = new  CComObjectNoLock<CFSFolder>;
  CComPtr<IFolderFolder> subFolder = fsFolderSpec;
  RETURN_IF_NOT_S_OK(fsFolderSpec->Init(name, 0));
  *resultFolder = subFolder.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::BindToFolder(UINT32 index, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (index >= _drives.Size())
    return E_INVALIDARG;
  return BindToFolderSpec(_drives[index].FullSystemName, resultFolder);
}

STDMETHODIMP CFSDrives::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)
{
  return BindToFolderSpec(GetSystemString(name, GetCurrentFileCodePage()), resultFolder);
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

STDMETHODIMP CFSDrives::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
}

STDMETHODIMP CFSDrives::GetTypeID(BSTR *name)
{
  CComBSTR temp = L"FSDrives";
  *name = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetPath(BSTR *path)
{
  CComBSTR temp = (LangLoadStringW(IDS_COMPUTER, 0x03020300)) + 
      UString(L'\\');
  *path = temp.Detach();
  return S_OK;
}

STDMETHODIMP CFSDrives::GetSystemIconIndex(UINT32 index, INT32 *iconIndex)
{
  const CDriveInfo &driveInfo = _drives[index];
  *iconIndex = GetRealIconIndex(0, driveInfo.FullSystemName);
  return S_OK;
}

