// StringUtils.h

#pragma once

#ifndef __STRINGUTILS_H
#define __STRINGUTILS_H

#include "Common/String.h"

void SplitStringToTwoStrings(const UString &src, UString &dest1, UString &dest2);

void SplitString(const UString &srcString, UStringVector &destStrings);
UString JoinStrings(const UStringVector &srcStrings);



#endif