// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "../../UI/GUI/OpenCallbackGUI.h"
#include "../../UI/Common/LoadCodecs.h"

HRESULT ExtractArchive(
    CCodecs *codecs,
    const UString &fileName, 
    const UString &folderName,
    COpenCallbackGUI *openCallback,
    bool showProgress,
    bool &isCorrupt, 
    UString &errorMessage);

#endif
