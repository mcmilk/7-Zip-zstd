// Windows/FileSystem.cpp

#include "StdAfx.h"

#include "Windows/FileSystem.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NFile {
namespace NSystem {

bool MyGetVolumeInformation(
    LPCTSTR rootPathName,
    CSysString &volumeName,
    LPDWORD volumeSerialNumber,
    LPDWORD maximumComponentLength,
    LPDWORD fileSystemFlags,
    CSysString &fileSystemName)
{
  bool result = BOOLToBool(GetVolumeInformation(
      rootPathName,
      volumeName.GetBuffer(MAX_PATH), MAX_PATH,
      volumeSerialNumber,
      maximumComponentLength,
      fileSystemFlags,
      fileSystemName.GetBuffer(MAX_PATH), MAX_PATH));
  volumeName.ReleaseBuffer();
  fileSystemName.ReleaseBuffer();
  return result;
}

typedef BOOL (WINAPI * GetDiskFreeSpaceExPointer)(
  LPCTSTR lpDirectoryName,                 // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

bool MyGetDiskFreeSpace(LPCTSTR rootPathName,
    UINT64 &clusterSize, UINT64 &totalSize, UINT64 &freeSize)
{
  GetDiskFreeSpaceExPointer pGetDiskFreeSpaceEx = 
      (GetDiskFreeSpaceExPointer)GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");

  bool sizeIsDetected = false;
  if (pGetDiskFreeSpaceEx)
  {
    UINT64 i64FreeBytesToCaller;
    sizeIsDetected = BOOLToBool(pGetDiskFreeSpaceEx(rootPathName,
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&totalSize,
                (PULARGE_INTEGER)&freeSize));
  }

  DWORD numSectorsPerCluster;
  DWORD bytesPerSector;
  DWORD numberOfFreeClusters;
  DWORD totalNumberOfClusters;

  if (!::GetDiskFreeSpace(rootPathName,
      &numSectorsPerCluster,
      &bytesPerSector,
      &numberOfFreeClusters,
      &totalNumberOfClusters))
    return false;

  clusterSize = UINT64(bytesPerSector) * UINT64(numSectorsPerCluster);
  if (!sizeIsDetected)
  {
    totalSize =  clusterSize * UINT64(totalNumberOfClusters);
    freeSize =  clusterSize * UINT64(numberOfFreeClusters);
  }
  return true;
}

}}}
