// ExtractEngine.h

#ifndef __EXTRACTENGINE_H
#define __EXTRACTENGINE_H

#include "Common/String.h"

HRESULT ExtractArchive(HWND parentWindow, const CSysString &fileName, 
    bool assumeYes = false);

#endif

