// Windows/SystemInfo.h

#ifndef __WINDOWS_SYSTEM_INFO_H
#define __WINDOWS_SYSTEM_INFO_H

#include "../Common/MyString.h"

void GetSystemInfoText(AString &s);
void PrintSize_KMGT_Or_Hex(AString &s, UInt64 v);
void Add_LargePages_String(AString &s);

#endif
