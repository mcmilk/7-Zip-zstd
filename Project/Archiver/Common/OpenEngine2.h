// OpenEngine2.h

#ifndef __OPENENGINE2_H
#define __OPENENGINE2_H

#include "FolderArchiveInterface.h"
#include "Common/String.h"
#include "ZipRegistryMain.h"

/*
HRESULT OpenArchive(const CSysString &aFileName, 
    IInFolderArchive **anArchiveHandler, 
    NZipRootRegistry::CArchiverInfo &anArchiverInfoResult);
*/

HRESULT OpenArchive(const CSysString &fileName, 
    IInFolderArchive **archiveHandler, 
    NZipRootRegistry::CArchiverInfo &archiverInfoResult,
    UString &defaultName,
    IArchiveOpenCallback *openArchiveCallback);

HRESULT ReOpenArchive(IInFolderArchive *archiveHandler,
    const UString &defaultName,
    const CSysString &fileName);


#endif
