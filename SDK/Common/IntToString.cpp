// Common/IntToString.cpp

#include "StdAfx.h"

#include "IntToString.h"

void ConvertUINT64ToString(UINT64 aValue, TCHAR *aString)
{
  TCHAR aTemp[32];
  int aPos = 0;
  do 
  {
    aTemp[aPos++] = TEXT('0') + int(aValue % 10);
    aValue /= 10;
  }
  while (aValue != 0);
  while(aPos > 0)
    *aString++ = aTemp[--aPos];
  *aString = TEXT('\0');
}

void ConvertINT64ToString(__int64 aValue, TCHAR *aString)
{
  if (aValue > 0)
    ConvertUINT64ToString(aValue, aString);
  else
  {
    *aString++ = '-';
    ConvertUINT64ToString(-aValue, aString);
  }
}

