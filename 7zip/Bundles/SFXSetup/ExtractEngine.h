// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "Common/String.h"

HRESULT ExtractArchive(
    const UString &fileName, 
    const UString &folderName
    #ifdef _SILENT
    , UString &resultMessage
    #endif
    );

#endif

