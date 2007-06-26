// StringUtils.h

#ifndef __STRINGUTILS_H
#define __STRINGUTILS_H

#include "Common/MyString.h"

void SplitStringToTwoStrings(const UString &src, UString &dest1, UString &dest2);

void SplitString(const UString &srcString, UStringVector &destStrings);
UString JoinStrings(const UStringVector &srcStrings);

#endif
