// OpenArchive.h

#ifndef __OPENARCHIVE_H
#define __OPENARCHIVE_H

#include "Common/String.h"

#include "../../Archive/IArchive.h"
#include "ArchiverInfo.h"

HRESULT OpenArchive(const UString &fileName, 
    #ifndef EXCLUDE_COM
    HMODULE *module,
    #endif
    IInArchive **archive, 
    CArchiverInfo &archiverInfoResult,
    int &subExtIndex,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT ReOpenArchive(IInArchive *archive, 
    const UString &fileName);


#endif
