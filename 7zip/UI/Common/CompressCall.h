// CompressCall.h

#ifndef __COMPRESSCALL_H
#define __COMPRESSCALL_H

#include "Common/String.h"
#include "Windows/Synchronization.h"

HRESULT MyCreateProcess(const UString &params,
   LPCTSTR lpCurrentDirectory,
   NWindows::NSynchronization::CEvent *event = NULL);
HRESULT CompressFiles(
    const UString &curDir,
    const UString &archiveName,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email, bool showDialog);

HRESULT ExtractArchives(
    const UStringVector &archivePaths,
    const UString &outFolder, bool showDialog);

HRESULT TestArchives(const UStringVector &archivePaths);

#endif

