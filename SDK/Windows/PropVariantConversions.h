// PropVariantConversions.h

#pragma once

#ifndef __PROPVARIANTCONVERSIONS_H
#define __PROPVARIANTCONVERSIONS_H

#include "Common/String.h"

CSysString ConvertFileTimeToString(const FILETIME &aFileTime, bool anIncludeTime = true);
CSysString ConvertPropVariantToString(const PROPVARIANT &aPropVariant);

UINT64 ConvertPropVariantToUINT64(const PROPVARIANT &aPropVariant);

#endif
