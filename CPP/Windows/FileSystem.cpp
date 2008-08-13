// Windows/FileSystem.cpp

#include "StdAfx.h"

#include "FileSystem.h"
#include "Defs.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

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


#ifndef _UNICODE
bool MyGetVolumeInformation(
    LPCWSTR rootPathName,
    UString &volumeName,
    LPDWORD volumeSerialNumber,
    LPDWORD maximumComponentLength,
    LPDWORD fileSystemFlags,
    UString &fileSystemName)
{
  if (g_IsNT)
  {
    bool result = BOOLToBool(GetVolumeInformationW(
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
  AString volumeNameA, fileSystemNameA;
  bool result = MyGetVolumeInformation(GetSystemString(rootPathName), volumeNameA,
      volumeSerialNumber, maximumComponentLength, fileSystemFlags,fileSystemNameA);
  if (result)
  {
    volumeName = GetUnicodeString(volumeNameA);
    fileSystemName = GetUnicodeString(fileSystemNameA);
  }
  return result;
}
#endif

typedef BOOL (WINAPI * GetDiskFreeSpaceExPointer)(
  LPCTSTR lpDirectoryName,                 // directory name
  PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
  PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
  PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
);

bool MyGetDiskFreeSpace(LPCTSTR rootPathName,
    UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize)
{
  GetDiskFreeSpaceExPointer pGetDiskFreeSpaceEx =
      (GetDiskFreeSpaceExPointer)GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), "GetDiskFreeSpaceExA");

  bool sizeIsDetected = false;
  if (pGetDiskFreeSpaceEx)
  {
    ULARGE_INTEGER i64FreeBytesToCaller, totalSize2, freeSize2;
    sizeIsDetected = BOOLToBool(pGetDiskFreeSpaceEx(rootPathName,
                &i64FreeBytesToCaller,
                &totalSize2,
                &freeSize2));
    totalSize = totalSize2.QuadPart;
    freeSize = freeSize2.QuadPart;
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

  clusterSize = (UInt64)bytesPerSector * (UInt64)numSectorsPerCluster;
  if (!sizeIsDetected)
  {
    totalSize =  clusterSize * (UInt64)totalNumberOfClusters;
    freeSize =  clusterSize * (UInt64)numberOfFreeClusters;
  }
  return true;
}

#ifndef _UNICODE
bool MyGetDiskFreeSpace(LPCWSTR rootPathName,
    UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize)
{
  return MyGetDiskFreeSpace(GetSystemString(rootPathName), clusterSize, totalSize, freeSize);
}
#endif

}}}
