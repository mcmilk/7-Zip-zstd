// AddSTD.h

#pragma once

#ifndef __ADDSTD_H
#define __ADDSTD_H

#include "Common/Wildcard.h"
#include "UpdateArchiveOptions.h"
// #include "ProxyHandler.h"
#include "../Common/IArchiveHandler2.h"
#include "Windows/FileFind.h"

HRESULT UpdateArchiveStdMain(const NWildcard::CCensor &aCensor, 
    CUpdateArchiveOptions &anOptions, const CSysString &aWorkingDir,
    IArchiveHandler200 *anArchive,
    const UString *aDefaultItemName,
    const NWindows::NFile::NFind::CFileInfo *anArchiveFileInfo,
    bool anEnablePercents);

#endif
