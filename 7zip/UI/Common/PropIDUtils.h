// PropIDUtils.h

#ifndef __PROPIDUTILS_H
#define __PROPIDUTILS_H

#include "Common/String.h"

UString ConvertPropertyToString(const PROPVARIANT &aPropVariant, 
    PROPID aPropID, bool aFull = true);

#endif
