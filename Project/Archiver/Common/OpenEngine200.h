// OpenEngine200.h

#ifndef __OPENENGINE200_H
#define __OPENENGINE200_H

#include "Common/String.h"

#include "FolderArchiveInterface.h"
#include "ZipRegistryMain.h"

HRESULT OpenArchive(const CSysString &fileName, 
    IInArchive **archive, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult);

HRESULT OpenArchive(const CSysString &fileName, 
    IInArchive **archive, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT ReOpenArchive(IInArchive *archive, 
    const CSysString &fileName);


#endif
