// CompressCall.h

#ifndef __COMPRESSCALL_H
#define __COMPRESSCALL_H

#include "Common/String.h"
#include "Windows/Synchronization.h"

HRESULT MyCreateProcess(const CSysString &params, 
    NWindows::NSynchronization::CEvent *event = NULL);
HRESULT CompressFiles(const CSysString &archiveName,
    const UStringVector &names, 
    // const UString &outFolder, 
    bool email, bool showDialog);

HRESULT ExtractArchive(const CSysString &archiveName,
    const CSysString &outFolder, bool showDialog);

HRESULT TestArchive(const CSysString &archiveName);

#endif

