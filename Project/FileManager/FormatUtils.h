// FormatUtils.h

#pragma once

#ifndef __FORMATUTILS_H
#define __FORMATUTILS_H

#include "Common/String.h"

// CSysString MyFormat(const CSysString &format, const CSysString &argument);

// CSysString NumberToString(UINT64 number);

UString NumberToStringW(UINT64 number);

UString MyFormatNew(const UString &format, const UString &argument);
UString MyFormatNew(UINT32 resourceID, 
    #ifdef LANG
    UINT32 aLangID, 
    #endif
    const UString &argument);

#endif
