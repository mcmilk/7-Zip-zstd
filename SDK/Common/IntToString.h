// Common/IntToString.h

#pragma once

#ifndef __COMMON_INTTOSTRING_H
#define __COMMON_INTTOSTRING_H

void ConvertUINT64ToWideString(UINT64 value, wchar_t *string);

void ConvertUINT64ToString(UINT64 value, TCHAR *aString);
void ConvertINT64ToString(INT64 value, TCHAR *aString);

#endif


