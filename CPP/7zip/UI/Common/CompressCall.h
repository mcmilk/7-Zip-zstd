// CompressCall.h

#ifndef __COMPRESSCALL_H
#define __COMPRESSCALL_H

#include "Common/MyString.h"
#include "Windows/Synchronization.h"

HRESULT MyCreateProcess(const UString &params,
   LPCWSTR lpCurrentDirectory, bool waitFinish,
   NWindows::NSynchronization::CBaseEvent *event);

HRESULT CompressFiles(
    const UString &curDir,
    const UString &archiveName,
    const UString &archiveType,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email, bool showDialog, bool waitFinish);

HRESULT ExtractArchives(
    const UStringVector &archivePaths,
    const UString &outFolder, bool showDialog);

HRESULT TestArchives(const UStringVector &archivePaths);

HRESULT Benchmark();

#endif

