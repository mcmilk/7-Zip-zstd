// Common/StringToInt.h

#pragma once

#ifndef __COMMON_STRINGTOINT_H
#define __COMMON_STRINGTOINT_H

UINT64 ConvertStringToUINT64(const char *s, const char **end);
UINT64 ConvertStringToUINT64(const wchar_t *s, const wchar_t **end);

INT64 ConvertStringToINT64(const char *s, const char **end);

#endif


