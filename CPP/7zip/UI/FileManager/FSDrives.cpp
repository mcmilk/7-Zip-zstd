// FSDrives.cpp

#include "StdAfx.h"

#include "../../../../C/Alloc.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/FileIO.h"
#include "Windows/FileSystem.h"
#include "Windows/PropVariant.h"

#include "../../PropID.h"

#include "FSDrives.h"
#include "FSFolder.h"
#include "LangUtils.h"
#include "SysIconUtils.h"

#include "resource.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

static const wchar_t *kVolPrefix = L"\\\\.\\";

UString CDriveInfo::GetDeviceFileIoName() const
{
  return kVolPrefix + Name;
}

struct CPhysTempBuffer
{
  void *buffer;
  CPhysTempBuffer(): buffer(0) {}
  ~CPhysTempBuffer() { MidFree(buffer); }
};

static HRESULT CopyFileSpec(LPCWSTR fromPath, LPCWSTR toPath, bool writeToDisk, UInt64 fileSize,
    UInt32 bufferSize, UInt64 progressStart, IProgress *progress)
{
  NFile::NIO::CInFile inFile;
  if (!inFile.Open(fromPath))
    return GetLastError();
  if (fileSize == (UInt64)(Int64)-1)
  {
    if (!inFile.GetLength(fileSize))
      ::GetLastError();
  }
  NFile::NIO::COutFile outFile;
  if (writeToDisk)
  {
    if (!outFile.Open(toPath, FILE_SHARE_WRITE, OPEN_EXISTING, 0))
      return GetLastError();
  }
  else
    if (!outFile.Create(toPath, true))
      return GetLastError();
  CPhysTempBuffer tempBuffer;
  tempBuffer.buffer = MidAlloc(bufferSize);
  if (tempBuffer.buffer == 0)
    return E_OUTOFMEMORY;
 
  for (UInt64 pos = 0; pos < fileSize;)
  {
    UInt64 progressCur = progressStart + pos;
    RINOK(progress->SetCompleted(&progressCur));
    UInt64 rem = fileSize - pos;
    UInt32 curSize = (UInt32)MyMin(rem, (UInt64)bufferSize);
    UInt32 processedSize;
    if (!inFile.Read(tempBuffer.buffer, curSize, processedSize))
      return GetLastError();
    if (processedSize == 0)
      break;
    curSize = processedSize;
    if (writeToDisk)
    {
      const UInt32 kMask = 0x1FF;
      curSize = (curSize + kMask) & ~kMask;
      if (curSize > bufferSize)
        return E_FAIL;
    }

    if (!outFile.Write(tempBuffer.buffer, curSize, processedSize))
      return GetLastError();
    if (curSize != processedSize)
      return E_FAIL;
    pos += curSize;
  }
  return S_OK;
}

static const STATPROPSTG kProps[] =
{
  { NULL, kpidName, VT_BSTR},
  { NULL, kpidTotalSize, VT_UI8},
  { NULL, kpidFreeSpace, VT_UI8},
  { NULL, kpidType, VT_BSTR},
  { NULL, kpidVolumeName, VT_BSTR},
  { NULL, kpidFileSystem, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI8}
};

static const char *kDriveTypes[] =
{
  "Unknown",
  "No Root Dir",
  "Removable",
  "Fixed",
  "Remote",
  "CD-ROM",
  "RAM disk"
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

    di.Name = di.FullSystemName.Left(di.FullSystemName.Length() - 1);
    di.ClusterSize = 0;
    di.DriveSize = 0;
    di.FreeSpace = 0;
    di.DriveType = NFile::NSystem::MyGetDriveType(driveName);
    bool needRead = true;
    if (di.DriveType == DRIVE_CDROM || di.DriveType == DRIVE_REMOVABLE)
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
    case kpidIsDir:  prop = !_volumeMode; break;
    case kpidName:  prop = di.Name; break;
    case kpidTotalSize:   if (di.KnownSizes) prop = di.DriveSize; break;
    case kpidFreeSpace:   if (di.KnownSizes) prop = di.FreeSpace; break;
    case kpidClusterSize: if (di.KnownSizes) prop = di.ClusterSize; break;
    case kpidType:
      if (di.DriveType < sizeof(kDriveTypes) / sizeof(kDriveTypes[0]))
        prop = kDriveTypes[di.DriveType];
      break;
    case kpidVolumeName:  prop = di.VolumeName; break;
    case kpidFileSystem:  prop = di.FileSystemName; break;
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CFSDrives::BindToFolderSpec(const wchar_t *name, IFolderFolder **resultFolder)
{
  *resultFolder = 0;
  if (_volumeMode)
    return S_OK;
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
  /*
  if (_volumeMode)
  {
    *resultFolder = 0;
    CPhysDriveFolder *folderSpec = new CPhysDriveFolder;
    CMyComPtr<IFolderFolder> subFolder = folderSpec;
    RINOK(folderSpec->Init(di.Name));
    *resultFolder = subFolder.Detach();
    return S_OK;
  }
  */
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

IMP_IFolderFolder_Props(CFSDrives)

STDMETHODIMP CFSDrives::GetFolderProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidType: prop = L"FSDrives"; break;
    case kpidPath:
      if (_volumeMode)
        prop = kVolPrefix;
      else
        prop = LangString(IDS_COMPUTER, 0x03020300) + UString(WCHAR_PATH_SEPARATOR);
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CFSDrives::GetSystemIconIndex(UInt32 index, Int32 *iconIndex)
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

UString CFSDrives::GetExt(int index) const
{
  const CDriveInfo &di = _drives[index];
  const wchar_t *ext = NULL;
  if (di.DriveType == DRIVE_CDROM)
    ext = L"iso";
  else if (di.FileSystemName.Find(L"NTFS") >= 0)
    ext = L"ntfs";
  else if (di.FileSystemName.Find(L"FAT") >= 0)
    ext = L"fat";
  else
    ext = L"img";
  return (UString)L'.' + ext;
}

HRESULT CFSDrives::GetLength(int index, UInt64 &length) const
{
  NFile::NIO::CInFile inFile;
  if (!inFile.Open(_drives[index].GetDeviceFileIoName()))
    return GetLastError();
  if (!inFile.LengthDefined)
    return E_FAIL;
  length = inFile.Length;
  return S_OK;
}

STDMETHODIMP CFSDrives::CopyTo(const UInt32 *indices, UInt32 numItems,
    const wchar_t *path, IFolderOperationsExtractCallback *callback)
{
  if (numItems == 0)
    return S_OK;
  
  if (!_volumeMode)
    return E_NOTIMPL;

  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
  {
    const CDriveInfo &di = _drives[indices[i]];
    if (di.KnownSizes)
      totalSize += di.DriveSize;
  }
  RINOK(callback->SetTotal(totalSize));
  RINOK(callback->SetNumFiles(numItems));
  
  UString destPath = path;
  if (destPath.IsEmpty())
    return E_INVALIDARG;
  bool directName = (destPath[destPath.Length() - 1] != WCHAR_PATH_SEPARATOR);
  if (directName)
  {
    if (numItems > 1)
      return E_INVALIDARG;
  }

  UInt64 completedSize = 0;
  RINOK(callback->SetCompleted(&completedSize));
  for (i = 0; i < numItems; i++)
  {
    int index = indices[i];
    const CDriveInfo &di = _drives[index];
    UString destPath2 = destPath;
    UString name = di.Name;
    if (!directName)
    {
      UString destName = name;
      if (!destName.IsEmpty() && destName[destName.Length() - 1] == L':')
      {
        destName.Delete(destName.Length() - 1);
        destName += GetExt(index);
      }
      destPath2 += destName;
    }
    UString srcPath = di.GetDeviceFileIoName();

    UInt64 fileSize = 0;
    if (GetLength(index, fileSize) != S_OK)
    {
      return E_FAIL;
    }
    if (!di.KnownSizes)
      totalSize += fileSize;
    RINOK(callback->SetTotal(totalSize));
    
    Int32 writeAskResult;
    CMyComBSTR destPathResult;
    RINOK(callback->AskWrite(srcPath, BoolToInt(false), NULL, &fileSize,
      destPath2, &destPathResult, &writeAskResult));
    if (!IntToBool(writeAskResult))
      continue;
    
    RINOK(callback->SetCurrentFilePath(srcPath));
    
    static const UInt32 kBufferSize = (4 << 20);
    UInt32 bufferSize = (di.DriveType == DRIVE_REMOVABLE) ? (18 << 10) * 4 : kBufferSize;
    RINOK(CopyFileSpec(srcPath, destPathResult, false, fileSize, bufferSize, completedSize, callback));
    completedSize += fileSize;
  }
  return S_OK;
}

STDMETHODIMP CFSDrives::MoveTo(
    const UInt32 * /* indices */,
    UInt32 /* numItems */,
    const wchar_t * /* path */,
    IFolderOperationsExtractCallback * /* callback */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::CopyFrom(const wchar_t * /* fromFolderPath */,
    const wchar_t ** /* itemsPaths */, UInt32 /* numItems */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::CreateFolder(const wchar_t * /* name */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::CreateFile(const wchar_t * /* name */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::Rename(UInt32 /* index */, const wchar_t * /* newName */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::Delete(const UInt32 * /* indices */, UInt32 /* numItems */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CFSDrives::SetProperty(UInt32 /* index */, PROPID /* propID */,
    const PROPVARIANT * /* value */, IProgress * /* progress */)
{
  return E_NOTIMPL;
}
