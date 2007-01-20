// List.h

#ifndef __LIST_H
#define __LIST_H

#include "Common/Wildcard.h"

HRESULT ListArchives(UStringVector &archivePaths, UStringVector &archivePathsFull,
    const NWildcard::CCensorNode &wildcardCensor,
    bool enableHeaders, bool techMode, bool &passwordEnabled, UString &password);

#endif

