// EnumDirItems.h

#pragma once

#ifndef __ENUM_DIR_ITEMS_H
#define __ENUM_DIR_ITEMS_H

#include "Common/String.h"
#include "Common/Vector.h"
#include "DirItem.h"
// #include "UpdatePairBasic.h"

#include "Windows/FileFind.h"

void AddDirFileInfo(
    const UString &prefix, 
    const CSysString &fullPathName,
    NWindows::NFile::NFind::CFileInfo &fileInfo, 
    CObjectVector<CDirItem> &dirItems,
    UINT codePage);

void EnumerateDirItems(
    const CSysString &baseFolderPrefix,
    const CSysStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems, 
    UINT codePage);

/*
void EnumerateItems(const CSysStringVector &filePaths,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage);
*/

#endif
