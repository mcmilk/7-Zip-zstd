// GUI/Extract.h

#ifndef __GUI_EXTRACT_H
#define __GUI_EXTRACT_H

#include "Common/String.h"

HRESULT ExtractArchive(HWND parentWindow, const CSysString &fileName, 
    bool assumeYes, bool showDialog, const CSysString &outputFolder);

#endif

