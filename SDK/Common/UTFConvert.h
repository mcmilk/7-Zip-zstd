// Common/UTFConvert.h

#pragma once

#ifndef __COMMON_UTFCONVERT_H
#define __COMMON_UTFCONVERT_H

#include "Common/String.h"

bool ConvertUTF8ToUnicode(const AString &anUTFString, UString &anResultString);
void ConvertUnicodeToUTF8(const UString &anUnicodeString, AString &anResultString);

#endif
