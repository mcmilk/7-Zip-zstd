// Windows/FileSystem.cpp

#include "StdAfx.h"

#include "Windows/FileSystem.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NFile {
namespace NSystem {

bool MyGetVolumeInformation(
    LPCTSTR aRootPathName,
    CSysString &aVolumeName,
    LPDWORD aVolumeSerialNumber,
    LPDWORD aMaximumComponentLength,
    LPDWORD aFileSystemFlags,
    CSysString &aFileSystemName)
{
  bool aResult = BOOLToBool(GetVolumeInformation(
      aRootPathName,
      aVolumeName.GetBuffer(MAX_PATH), MAX_PATH,
      aVolumeSerialNumber,
      aMaximumComponentLength,
      aFileSystemFlags,
      aFileSystemName.GetBuffer(MAX_PATH), MAX_PATH));
  aVolumeName.ReleaseBuffer();
  aFileSystemName.ReleaseBuffer();
  return aResult;
}

bool MyGetDiskFreeSpace(LPCTSTR aRootPathName,
    UINT64 &aClusterSize, UINT64 &aTotalSize, UINT64 &aFreeSize)
{
  FARPROC pGetDiskFreeSpaceEx = 
      GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");

  bool aSizeIsDetected = false;
  if (pGetDiskFreeSpaceEx)
  {
    UINT64 i64FreeBytesToCaller;
    aSizeIsDetected = BOOLToBool(::GetDiskFreeSpaceEx(aRootPathName,
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&aTotalSize,
                (PULARGE_INTEGER)&aFreeSize));
  }

  DWORD aSectorsPerCluster;     // sectors per cluster
  DWORD aBytesPerSector;        // bytes per sector
  DWORD aNumberOfFreeClusters;  // free clusters
  DWORD aTotalNumberOfClusters; // total clusters

  if (!::GetDiskFreeSpace(aRootPathName,
      &aSectorsPerCluster,
      &aBytesPerSector,
      &aNumberOfFreeClusters,
      &aTotalNumberOfClusters))
    return false;

  aClusterSize = UINT64(aBytesPerSector) * UINT64(aSectorsPerCluster);
  if (!aSizeIsDetected)
  {
    aTotalSize =  aClusterSize * UINT64(aTotalNumberOfClusters);
    aFreeSize =  aClusterSize * UINT64(aNumberOfFreeClusters);
  }
  return true;
}

}}}
