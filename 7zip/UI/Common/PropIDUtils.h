// PropIDUtils.h

#pragma once

#ifndef __PROPIDUTILS_H
#define __PROPIDUTILS_H

#include "Common/String.h"

CSysString ConvertPropertyToString(const PROPVARIANT &aPropVariant, PROPID aPropID, 
    bool aFull = true);


#endif


