// Common/String.cpp

#include "StdAfx.h"

#include "String.h"
#include "StringConvert.h"

#ifndef _UNICODE

wchar_t MyCharUpper(wchar_t c)
{
  if (c == 0)
    return 0;
  wchar_t *res = CharUpperW((LPWSTR)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)res;
  const int kBufferSize = 4;
  char s[kBufferSize];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufferSize, 0, 0);
  ::CharUpperA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}

wchar_t MyCharLower(wchar_t c)
{
  if (c == 0)
    return 0;
  wchar_t *res = CharLowerW((LPWSTR)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)res;
  const int kBufferSize = 4;
  char s[kBufferSize];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufferSize, 0, 0);
  ::CharLowerA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}

wchar_t * MyStringUpper(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *res = CharUpperW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeUpper();
  return MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
}

wchar_t * MyStringLower(wchar_t *s)
{ 
  if (s == 0)
    return 0;
  wchar_t *res = CharLowerW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeLower();
  return MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
}

int MyStringCollate(const wchar_t *s1, const wchar_t *s2)
{ 
  int res = CompareStringW(
        LOCALE_USER_DEFAULT, SORT_STRINGSORT, s1, -1, s2, -1); 
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return ConvertCompareResult(res);
  return MyStringCollate(UnicodeStringToMultiByte(s1), 
        UnicodeStringToMultiByte(s2));
}

int MyStringCollateNoCase(const wchar_t *s1, const wchar_t *s2)
{ 
  int res = CompareStringW(
        LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, s1, -1, s2, -1); 
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return ConvertCompareResult(res);
  return MyStringCollateNoCase(UnicodeStringToMultiByte(s1), 
      UnicodeStringToMultiByte(s2));
}

#endif
