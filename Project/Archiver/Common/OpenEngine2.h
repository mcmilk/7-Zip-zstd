// OpenEngine2.h

#ifndef __OPENENGINE2_H
#define __OPENENGINE2_H

#include "IArchiveHandler2.h"
#include "Common/String.h"
#include "ZipRegistryMain.h"

/*
HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler100 **anArchiveHandler, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult);
*/

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler100 **anArchiveHandler, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    UString &aDefaultName,
    IOpenArchive2CallBack *anOpenArchive2CallBack);

HRESULT ReOpenArchive(IArchiveHandler100 *anArchiveHandler,
    const UString &aDefaultName,
    const CSysString &aFileName);


#endif
