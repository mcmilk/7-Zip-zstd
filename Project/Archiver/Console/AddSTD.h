// AddSTD.h

#pragma once

#ifndef __ADDSTD_H
#define __ADDSTD_H

#include "Common/Wildcard.h"
#include "UpdateArchiveOptions.h"
// #include "ProxyHandler.h"
#include "Windows/FileFind.h"
#include "../Format/Common/ArchiveInterface.h"

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &censor, 
    CUpdateArchiveOptions &options, const CSysString &workingDir,
    IInArchive *archive,
    const UString *defaultItemName,
    const NWindows::NFile::NFind::CFileInfo *archiveFileInfo,
    bool enablePercents);

#endif
