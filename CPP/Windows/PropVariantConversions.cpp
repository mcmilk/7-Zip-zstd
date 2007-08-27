// PropVariantConversions.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "PropVariantConversions.h"

#include "Windows/Defs.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"

static UString ConvertUInt64ToString(UInt64 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

static UString ConvertInt64ToString(Int64 value)
{
  wchar_t buffer[32];
  ConvertInt64ToString(value, buffer);
  return buffer;
}

static char *UIntToStringSpec(UInt32 value, char *s, int numPos)
{
  char temp[16];
  int pos = 0;
  do 
  {
    temp[pos++] = (char)('0' + value % 10);
    value /= 10;
  }
  while (value != 0);
  int i;
  for (i = 0; i < numPos - pos; i++)
    *s++ = '0';
  do
    *s++ = temp[--pos];
  while (pos > 0);
  *s = '\0';
  return s;
}

bool ConvertFileTimeToString(const FILETIME &ft, char *s, bool includeTime, bool includeSeconds)
{
  s[0] = '\0';
  SYSTEMTIME st;
  if(!BOOLToBool(FileTimeToSystemTime(&ft, &st)))
    return false;
  s = UIntToStringSpec(st.wYear, s, 4);
  *s++ = '-';
  s = UIntToStringSpec(st.wMonth, s, 2);
  *s++ = '-';
  s = UIntToStringSpec(st.wDay, s, 2);
  if (includeTime)
  {
    *s++ = ' ';
    s = UIntToStringSpec(st.wHour, s, 2);
    *s++ = ':';
    s = UIntToStringSpec(st.wMinute, s, 2);
    if (includeSeconds)
    {
      *s++ = ':';
      UIntToStringSpec(st.wSecond, s, 2);
    }
  }
  /*
  sprintf(s, "%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
  if (includeTime)
  {
    sprintf(s + strlen(s), " %02d:%02d", st.wHour, st.wMinute);
    if (includeSeconds)
      sprintf(s + strlen(s), ":%02d", st.wSecond);
  }
  */
  return true;
}

UString ConvertFileTimeToString(const FILETIME &fileTime, bool includeTime, bool includeSeconds)
{
  char s[32];
  ConvertFileTimeToString(fileTime, s,  includeTime, includeSeconds);
  return GetUnicodeString(s);
}
 

UString ConvertPropVariantToString(const PROPVARIANT &propVariant)
{
  switch (propVariant.vt)
  {
    case VT_EMPTY:
      return UString();
    case VT_BSTR:
      return propVariant.bstrVal;
    case VT_UI1:
      return ConvertUInt64ToString(propVariant.bVal);
    case VT_UI2:
      return ConvertUInt64ToString(propVariant.uiVal);
    case VT_UI4:
      return ConvertUInt64ToString(propVariant.ulVal);
    case VT_UI8:
      return ConvertUInt64ToString(propVariant.uhVal.QuadPart);
    case VT_FILETIME:
      return ConvertFileTimeToString(propVariant.filetime, true, true);
    /*
    case VT_I1:
      return ConvertInt64ToString(propVariant.cVal);
    */
    case VT_I2:
      return ConvertInt64ToString(propVariant.iVal);
    case VT_I4:
      return ConvertInt64ToString(propVariant.lVal);
    case VT_I8:
      return ConvertInt64ToString(propVariant.hVal.QuadPart);

    case VT_BOOL:
      return VARIANT_BOOLToBool(propVariant.boolVal) ? L"+" : L"-";
    default:
      #ifndef _WIN32_WCE
      throw 150245;
      #else
      return UString();
      #endif
  }
}

UInt64 ConvertPropVariantToUInt64(const PROPVARIANT &propVariant)
{
  switch (propVariant.vt)
  {
    case VT_UI1:
      return propVariant.bVal;
    case VT_UI2:
      return propVariant.uiVal;
    case VT_UI4:
      return propVariant.ulVal;
    case VT_UI8:
      return (UInt64)propVariant.uhVal.QuadPart;
    default:
      #ifndef _WIN32_WCE
      throw 151199;
      #else
      return 0;
      #endif
  }
}
