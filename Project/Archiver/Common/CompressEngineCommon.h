// CompressEngineCommon.h

#pragma once

#ifndef __COMPRESSENGINECOMMON_H
#define __COMPRESSENGINECOMMON_H

#include "Common/String.h"

#include "ArchiveStyleDirItemInfo.h"
#include "UpdatePairBasic.h"

#include "Windows/FileFind.h"

void AddDirFileInfo(const UString &aPrefix, const CSysString &aFullPathName,
    NWindows::NFile::NFind::CFileInfo &aFileInfo, 
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector,
    UINT aCodePage);

void EnumerateItems(const CSysStringVector &aFileNames,
    const UString &anArchiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &aDirFileInfoVector, UINT aCodePage);

extern const NUpdateArchive::CActionSet kAddActionSet;
extern const NUpdateArchive::CActionSet kUpdateActionSet;
extern const NUpdateArchive::CActionSet kFreshActionSet;
extern const NUpdateArchive::CActionSet kSynchronizeActionSet;
extern const NUpdateArchive::CActionSet kDeleteActionSet;

#endif
