// Windows/FileSystem.h

#ifndef __WINDOWS_FILESYSTEM_H
#define __WINDOWS_FILESYSTEM_H

#include "../Common/MyString.h"
#include "../Common/Types.h"

#ifndef _UNICODE
#include "../Common/StringConvert.h"
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
    CSysString &fileSystemName);

#ifndef _UNICODE
bool MyGetVolumeInformation(
    LPCWSTR rootPathName,
    UString &volumeName,
    LPDWORD volumeSerialNumber,
    LPDWORD maximumComponentLength,
    LPDWORD fileSystemFlags,
    UString &fileSystemName);
#endif

inline UINT MyGetDriveType(LPCTSTR pathName) { return GetDriveType(pathName); }
#ifndef _UNICODE
inline UINT MyGetDriveType(LPCWSTR pathName) { return GetDriveType(GetSystemString(pathName)); }
#endif

bool MyGetDiskFreeSpace(LPCTSTR rootPathName,
    UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);

#ifndef _UNICODE
bool MyGetDiskFreeSpace(LPCWSTR rootPathName,
    UInt64 &clusterSize, UInt64 &totalSize, UInt64 &freeSize);
#endif

}}}

#endif

