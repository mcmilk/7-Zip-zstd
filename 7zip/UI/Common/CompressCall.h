// CompressCall.h

#ifndef __COMPRESSCALL_H
#define __COMPRESSCALL_H

#include "Common/String.h"
#include "Windows/Synchronization.h"

HRESULT MyCreateProcess(const UString &params, 
    NWindows::NSynchronization::CEvent *event = NULL);
HRESULT CompressFiles(const UString &archiveName,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email, bool showDialog);

HRESULT ExtractArchive(const UString &archiveName,
    const UString &outFolder, bool showDialog);

HRESULT TestArchive(const UString &archiveName);

#endif

