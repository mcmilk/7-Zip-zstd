// Windows/PropVariantConversions.h

#pragma once

#ifndef __PROPVARIANTCONVERSIONS_H
#define __PROPVARIANTCONVERSIONS_H

#include "Common/String.h"

// CSysString ConvertFileTimeToString(const FILETIME &fileTime, bool includeTime = true);
CSysString ConvertFileTimeToString2(const FILETIME &fileTime, bool includeTime = true, 
    bool includeSeconds = true);
CSysString ConvertPropVariantToString(const PROPVARIANT &propVariant);

UINT64 ConvertPropVariantToUINT64(const PROPVARIANT &propVariant);

#endif
