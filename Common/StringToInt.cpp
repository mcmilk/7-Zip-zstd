// Common/StringToInt.cpp

#include "StdAfx.h"

#include "StringToInt.h"

UINT64 ConvertStringToUINT64(const char *s, const char **end)
{
  UINT64 result = 0;
  while(true)
  {
    char c = *s;
    if (c < '0' || c > '9')
    {
      if (end != NULL)
        *end = s;
      return result;
    }
    result *= 10;
    result += (c - '0');
    s++;
  }
}

UINT64 ConvertStringToUINT64(const wchar_t *s, const wchar_t **end)
{
  UINT64 result = 0;
  while(true)
  {
    wchar_t c = *s;
    if (c < '0' || c > '9')
    {
      if (end != NULL)
        *end = s;
      return result;
    }
    result *= 10;
    result += (c - '0');
    s++;
  }
}


INT64 ConvertStringToINT64(const char *s, const char **end)
{
  INT64 result = 0;
  if (*s == '-')
    return -(INT64)ConvertStringToUINT64(s + 1, end);
  return ConvertStringToUINT64(s, end);
}
