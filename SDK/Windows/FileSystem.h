// Windows/FileSystem.h

#pragma once

#ifndef __WINDOWS_FILESYSTEM_H
#define __WINDOWS_FILESYSTEM_H

#include "Common/String.h"

namespace NWindows {
namespace NFile {
namespace NSystem {

bool MyGetVolumeInformation(
    LPCTSTR aRootPathName,
    CSysString &aVolumeName,
    LPDWORD aVolumeSerialNumber,
    LPDWORD aMaximumComponentLength,
    LPDWORD aFileSystemFlags,
    CSysString &aFileSystemName);

bool MyGetDiskFreeSpace(LPCTSTR aRootPathName,
    UINT64 &aClusterSize, UINT64 &aTotalSize, UINT64 &aFreeSize);

}}}

#endif

