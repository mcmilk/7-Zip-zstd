// EnumDirItems.h

#ifndef __ENUM_DIR_ITEMS_H
#define __ENUM_DIR_ITEMS_H

#include "Common/Wildcard.h"
#include "DirItem.h"

#include "Windows/FileFind.h"

void AddDirFileInfo(
    const UString &prefix, 
    const UString &fullPathName,
    const NWindows::NFile::NFind::CFileInfoW &fileInfo, 
    CObjectVector<CDirItem> &dirItems);


HRESULT EnumerateDirItems(
    const UString &baseFolderPrefix,
    const UStringVector &fileNames,
    const UString &archiveNamePrefix, 
    CObjectVector<CDirItem> &dirItems, 
    UString &errorPath);

struct IEnumDirItemCallback
{
  virtual HRESULT CheckBreak() { return  S_OK; }
};


HRESULT EnumerateItems(
    const NWildcard::CCensor &censor, 
    CObjectVector<CDirItem> &dirItems, 
    IEnumDirItemCallback *callback, 
    UString &errorPath);

#endif
