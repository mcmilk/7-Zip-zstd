// CompressEngineCommon.h

#pragma once

#ifndef __COMPRESSENGINECOMMON_H
#define __COMPRESSENGINECOMMON_H

#include "Common/String.h"

#include "ArchiveStyleDirItemInfo.h"
#include "UpdatePairBasic.h"

#include "Windows/FileFind.h"

void AddDirFileInfo(const UString &prefix, const CSysString &fullPathName,
    NWindows::NFile::NFind::CFileInfo &fileInfo, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector,
    UINT codePage);

void EnumerateItems(const CSysString &baseFolderPrefix,
    const CSysStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage);

/*
void EnumerateItems(const CSysStringVector &filePaths,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage);
*/

extern const NUpdateArchive::CActionSet kAddActionSet;
extern const NUpdateArchive::CActionSet kUpdateActionSet;
extern const NUpdateArchive::CActionSet kFreshActionSet;
extern const NUpdateArchive::CActionSet kSynchronizeActionSet;
extern const NUpdateArchive::CActionSet kDeleteActionSet;

#endif
