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
    const UString &fullPathName,
    NWindows::NFile::NFind::CFileInfoW &fileInfo, 
    CObjectVector<CDirItem> &dirItems);

void EnumerateDirItems(
    const UString &baseFolderPrefix,
    const UStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems);

/*
void EnumerateItems(const CSysStringVector &filePaths,
    const UString &archiveNamePrefix, 
    CArchiveStyleDirItemInfoVector &dirFileInfoVector, UINT codePage);
*/

#endif
