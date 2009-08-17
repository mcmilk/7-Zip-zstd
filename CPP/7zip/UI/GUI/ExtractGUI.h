// GUI/ExtractGUI.h

#ifndef __EXTRACT_GUI_H
#define __EXTRACT_GUI_H

#include "../Common/Extract.h"

#include "../FileManager/ExtractCallback.h"

/*
  RESULT can be S_OK, even if there are errors!!!
  if RESULT == S_OK, check extractCallback->IsOK() after ExtractGUI().

  RESULT = E_ABORT - user break.
  RESULT != E_ABORT:
  {
   messageWasDisplayed = true  - message was displayed already.
   messageWasDisplayed = false - there was some internal error, so you must show error message.
  }
*/

HRESULT ExtractGUI(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    UStringVector &archivePaths,
    UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    CExtractOptions &options,
    bool showDialog,
    bool &messageWasDisplayed,
    CExtractCallbackImp *extractCallback,
    HWND hwndParent = NULL);

#endif
