// MyOpenArchive.h

#ifndef __MYOPENARCHIVE_H
#define __MYOPENARCHIVE_H

#include "Windows/FileFind.h"
#include "../../Archive/IArchive.h"

HRESULT MyOpenArchive(const UString &archiveName, 
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    #ifndef EXCLUDE_COM
    HMODULE *module,
    #endif
    IInArchive **archive,
    UString &defaultItemName,
    bool &passwordEnabled, 
    UString &password);

HRESULT MyOpenArchive(const UString &archiveName, 
    const NWindows::NFile::NFind::CFileInfoW &archiveFileInfo,
    #ifndef EXCLUDE_COM
    HMODULE *module0,
    HMODULE *module1,
    #endif
    IInArchive **archive0,
    IInArchive **archive1,
    UString &defaultItemName0,
    UString &defaultItemName1,
    bool &passwordEnabled, 
    UString &password);

#endif
  