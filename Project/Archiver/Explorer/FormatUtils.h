// FormatUtils.h

#pragma once

#ifndef __FORMATUTILS_H
#define __FORMATUTILS_H

#include "Common/String.h"

CSysString MyFormat(const CSysString &aFormat, const CSysString &aString);
CSysString MyFormat(UINT32 anID, const CSysString &aString);
CSysString NumberToString(UINT64 aNumber);

#endif
