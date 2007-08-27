// GUI/UpdateGUI.h

#ifndef __UPDATE_GUI_H
#define __UPDATE_GUI_H

#include "../Common/Update.h"
#include "OpenCallbackGUI.h"
#include "UpdateCallbackGUI.h"

#include "../FileManager/UpdateCallback100.h"

HRESULT UpdateGUI(
    CCodecs *codecs,
    const NWildcard::CCensor &censor, 
    CUpdateOptions &options,
    bool showDialog,
    CUpdateErrorInfo &errorInfo,
    COpenCallbackGUI *openCallback,
    CUpdateCallbackGUI *callback);

#endif
