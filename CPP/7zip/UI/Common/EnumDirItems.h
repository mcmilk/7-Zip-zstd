// EnumDirItems.h

#ifndef __ENUM_DIR_ITEMS_H
#define __ENUM_DIR_ITEMS_H

#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileFind.h"

#include "DirItem.h"

void AddDirFileInfo(int phyParent, int logParent, int secureIndex,
    const NWindows::NFile::NFind::CFileInfo &fi, CObjectVector<CDirItem> &dirItems);

struct IEnumDirItemCallback
{
  virtual HRESULT ScanProgress(UInt64 numFolders, UInt64 numFiles, UInt64 totalSize, const wchar_t *path, bool isDir) = 0;
};

HRESULT EnumerateItems(
    const NWildcard::CCensor &censor,
    NWildcard::ECensorPathMode pathMode,
    const UString &addPathPrefix,
    CDirItems &dirItems,
    IEnumDirItemCallback *callback);

#endif
