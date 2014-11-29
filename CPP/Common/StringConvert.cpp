// Common/StringConvert.cpp

#include "StdAfx.h"

#include "StringConvert.h"

#ifndef _WIN32
#include <stdlib.h>
#endif

#ifdef _WIN32
UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
  UString resultString;
  if (!srcString.IsEmpty())
  {
    int numChars = MultiByteToWideChar(codePage, 0, srcString,
        srcString.Len(), resultString.GetBuffer(srcString.Len()),
        srcString.Len() + 1);
    if (numChars == 0)
      throw 282228;
    resultString.ReleaseBuffer(numChars);
  }
  return resultString;
}

void MultiByteToUnicodeString2(UString &dest, const AString &srcString, UINT codePage)
{
  dest.Empty();
  if (!srcString.IsEmpty())
  {
    wchar_t *destBuf = dest.GetBuffer(srcString.Len());
    const char *sp = (const char *)srcString;
    unsigned i;
    for (i = 0;;)
    {
      char c = sp[i];
      if ((Byte)c >= 0x80 || c == 0)
        break;
      destBuf[i++] = (wchar_t)c;
    }

    if (i != srcString.Len())
    {
      unsigned numChars = MultiByteToWideChar(codePage, 0, sp + i,
          srcString.Len() - i, destBuf + i,
          srcString.Len() + 1 - i);
      if (numChars == 0)
        throw 282228;
      i += numChars;
    }
    dest.ReleaseBuffer(i);
  }
}

void UnicodeStringToMultiByte2(AString &dest, const UString &s, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
  dest.Empty();
  defaultCharWasUsed = false;
  if (!s.IsEmpty())
  {
    unsigned numRequiredBytes = s.Len() * 2;
    char *destBuf = dest.GetBuffer(numRequiredBytes);
    unsigned i;
    const wchar_t *sp = (const wchar_t *)s;
    for (i = 0;;)
    {
      wchar_t c = sp[i];
      if (c >= 0x80 || c == 0)
        break;
      destBuf[i++] = (char)c;
    }
    defaultCharWasUsed = false;
    if (i != s.Len())
    {
      BOOL defUsed;
      unsigned numChars = WideCharToMultiByte(codePage, 0, sp + i, s.Len() - i,
          destBuf + i, numRequiredBytes + 1 - i,
          &defaultChar, &defUsed);
      defaultCharWasUsed = (defUsed != FALSE);
      if (numChars == 0)
        throw 282229;
      i += numChars;
    }
    dest.ReleaseBuffer(i);
  }
}

void UnicodeStringToMultiByte2(AString &dest, const UString &srcString, UINT codePage)
{
  bool defaultCharWasUsed;
  UnicodeStringToMultiByte2(dest, srcString, codePage, '_', defaultCharWasUsed);
}

AString UnicodeStringToMultiByte(const UString &s, UINT codePage, char defaultChar, bool &defaultCharWasUsed)
{
  AString dest;
  defaultCharWasUsed = false;
  if (!s.IsEmpty())
  {
    unsigned numRequiredBytes = s.Len() * 2;
    BOOL defUsed;
    int numChars = WideCharToMultiByte(codePage, 0, s, s.Len(),
        dest.GetBuffer(numRequiredBytes), numRequiredBytes + 1,
        &defaultChar, &defUsed);
    defaultCharWasUsed = (defUsed != FALSE);
    if (numChars == 0)
      throw 282229;
    dest.ReleaseBuffer(numChars);
  }
  return dest;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
  bool defaultCharWasUsed;
  return UnicodeStringToMultiByte(srcString, codePage, '_', defaultCharWasUsed);
}

#ifndef UNDER_CE
AString SystemStringToOemString(const CSysString &srcString)
{
  AString result;
  CharToOem(srcString, result.GetBuffer(srcString.Len() * 2));
  result.ReleaseBuffer();
  return result;
}
#endif

#else

UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
  UString resultString;
  for (unsigned i = 0; i < srcString.Len(); i++)
    resultString += (wchar_t)srcString[i];
  /*
  if (!srcString.IsEmpty())
  {
    int numChars = mbstowcs(resultString.GetBuffer(srcString.Len()), srcString, srcString.Len() + 1);
    if (numChars < 0) throw "Your environment does not support UNICODE";
    resultString.ReleaseBuffer(numChars);
  }
  */
  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
  AString resultString;
  for (unsigned i = 0; i < srcString.Len(); i++)
    resultString += (char)srcString[i];
  /*
  if (!srcString.IsEmpty())
  {
    int numRequiredBytes = srcString.Len() * 6 + 1;
    int numChars = wcstombs(resultString.GetBuffer(numRequiredBytes), srcString, numRequiredBytes);
    if (numChars < 0) throw "Your environment does not support UNICODE";
    resultString.ReleaseBuffer(numChars);
  }
  */
  return resultString;
}

#endif
