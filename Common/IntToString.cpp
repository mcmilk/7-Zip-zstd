// Common/IntToString.cpp

#include "StdAfx.h"

#include "IntToString.h"

void ConvertUInt64ToString(UInt64 value, char *s)
{
  char temp[32];
  int pos = 0;
  do 
  {
    temp[pos++] = '0' + int(value % 10);
    value /= 10;
  }
  while (value != 0);
  while(pos > 0)
    *s++ = temp[--pos];
  *s = L'\0';
}

void ConvertUInt64ToString(UInt64 value, wchar_t *s)
{
  wchar_t temp[32];
  int pos = 0;
  do 
  {
    temp[pos++] = L'0' + int(value % 10);
    value /= 10;
  }
  while (value != 0);
  while(pos > 0)
    *s++ = temp[--pos];
  *s = L'\0';
}

void ConvertInt64ToString(Int64 value, char *s)
{
  if (value >= 0)
    ConvertUInt64ToString(value, s);
  else
  {
    *s++ = '-';
    ConvertUInt64ToString(-value, s);
  }
}

void ConvertInt64ToString(Int64 value, wchar_t *s)
{
  if (value >= 0)
    ConvertUInt64ToString(value, s);
  else
  {
    *s++ = L'-';
    ConvertUInt64ToString(-value, s);
  }
}
