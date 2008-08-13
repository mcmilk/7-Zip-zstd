// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "../../UI/Common/LoadCodecs.h"

HRESULT ExtractArchive(CCodecs *codecs, const UString &fileName, const UString &destFolder,
    bool showProgress, bool &isCorrupt,  UString &errorMessage);

#endif
