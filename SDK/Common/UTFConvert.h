// UTFConvert.h

#pragma once

#ifndef __UTFCONVERT_H
#define __UTFCONVERT_H

#include "Common/String.h"

bool ConvertUTF8ToUnicode(const AString &anUTFString, UString &anResultString);
void ConvertUnicodeToUTF(const UString &anUnicodeString, AString &anResultString);

#endif
