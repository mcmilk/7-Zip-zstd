// OpenEngine200.h

#ifndef __OPENENGINE200_H
#define __OPENENGINE200_H

#include "Common/String.h"

#include "IArchiveHandler2.h"
#include "ZipRegistryMain.h"

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler200 **anArchiveHandler, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult);

HRESULT OpenArchive(const CSysString &aFileName, 
    IArchiveHandler200 **anArchiveHandler, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult,
    IOpenArchive2CallBack *anOpenArchive2CallBack);

HRESULT ReOpenArchive(IArchiveHandler200 *anArchiveHandler, 
    const CSysString &aFileName);


#endif
