// Common/ListFileUtils.h

#pragma once

#ifndef __COMMON_LISTFILEUTILS_H
#define __COMMON_LISTFILEUTILS_H

#include "Common/String.h"
#include "Common/Types.h"

bool ReadNamesFromListFile(LPCTSTR fileName, UStringVector &strings, 
    UINT codePage = CP_OEMCP);

#endif
