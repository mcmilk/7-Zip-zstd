// Windows/PropVariantConversions.h

#ifndef __PROPVARIANTCONVERSIONS_H
#define __PROPVARIANTCONVERSIONS_H

#include "Common/String.h"

// CSysString ConvertFileTimeToString(const FILETIME &fileTime, bool includeTime = true);
UString ConvertFileTimeToString2(const FILETIME &fileTime, bool includeTime = true, 
    bool includeSeconds = true);
UString ConvertPropVariantToString(const PROPVARIANT &propVariant);

UINT64 ConvertPropVariantToUINT64(const PROPVARIANT &propVariant);

#endif
