// Common/IntToString.h

#pragma once

#ifndef __COMMON_INTTOSTRING_H
#define __COMMON_INTTOSTRING_H

#include "types.h"

void ConvertUINT64ToString(UINT64 value, char *s);
void ConvertUINT64ToString(UINT64 value, wchar_t *s);

void ConvertINT64ToString(INT64 value, char *s);
void ConvertINT64ToString(INT64 value, wchar_t *s);

#endif


