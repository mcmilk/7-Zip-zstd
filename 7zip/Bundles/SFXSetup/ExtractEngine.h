// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "Common/String.h"

HRESULT ExtractArchive(
    const CSysString &fileName, 
    const CSysString &folderName
    #ifdef _SILENT
    , CSysString &resultMessage
    #endif
    );

#endif

