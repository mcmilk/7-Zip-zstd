// GUI/Extract.h

#ifndef __GUI_EXTRACT_H
#define __GUI_EXTRACT_H

#include "Common/String.h"

HRESULT ExtractArchive(HWND parentWindow, const UString &fileName, 
    bool assumeYes, bool showDialog, const UString &outputFolder);

#endif

