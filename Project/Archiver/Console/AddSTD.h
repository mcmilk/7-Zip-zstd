// AddSTD.h

#pragma once

#ifndef __ADDSTD_H
#define __ADDSTD_H

#include "Common/Wildcard.h"
#include "UpdateArchiveOptions.h"
// #include "ProxyHandler.h"
#include "../Common/IArchiveHandler2.h"
#include "Windows/FileFind.h"

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &censor, 
    CUpdateArchiveOptions &options, const CSysString &workingDir,
    IArchiveHandler200 *archive,
    const UString *defaultItemName,
    const NWindows::NFile::NFind::CFileInfo *archiveFileInfo,
    bool enablePercents);

#endif
