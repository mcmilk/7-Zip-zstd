// Common/IntToString.cpp

#include "StdAfx.h"

#include "IntToString.h"

void ConvertUINT64ToWideString(UINT64 value, wchar_t *string)
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
    *string++ = temp[--pos];
  *string = L'\0';
}

void ConvertUINT64ToString(UINT64 value, TCHAR *string)
{
  TCHAR temp[32];
  int pos = 0;
  do 
  {
    temp[pos++] = TEXT('0') + int(value % 10);
    value /= 10;
  }
  while (value != 0);
  while(pos > 0)
    *string++ = temp[--pos];
  *string = TEXT('\0');
}

void ConvertINT64ToString(__int64 value, TCHAR *string)
{
  if (value > 0)
    ConvertUINT64ToString(value, string);
  else
  {
    *string++ = '-';
    ConvertUINT64ToString(-value, string);
  }
}

