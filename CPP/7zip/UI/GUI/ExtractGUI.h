// GUI/ExtractGUI.h

#ifndef __EXTRACT_GUI_H
#define __EXTRACT_GUI_H

#include "../Common/Extract.h"

#include "../FileManager/ExtractCallback.h"

HRESULT ExtractGUI(
    CCodecs *codecs,
    const CIntVector &formatIndices,
    UStringVector &archivePaths,
    UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    CExtractOptions &options,
    bool showDialog,
    CExtractCallbackImp *extractCallback);

#endif
