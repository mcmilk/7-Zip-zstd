// Windows/FileSystem.h

#pragma once

#ifndef __WINDOWS_FILESYSTEM_H
#define __WINDOWS_FILESYSTEM_H

#include "Common/String.h"

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

bool MyGetDiskFreeSpace(LPCTSTR rootPathName,
    UINT64 &clusterSize, UINT64 &totalSize, UINT64 &freeSize);

}}}

#endif

