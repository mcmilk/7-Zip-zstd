// UTFConvert.cpp

#include "StdAfx.h"

#include "UTFConvert.h"

bool ConvertUTF8ToUnicode(const AString &utfString, UString &resultString)
{
  resultString.Empty();
  for(int i = 0; i < utfString.Length(); i++)
  {
    BYTE c = utfString[i];
    if (c < 0x80)
    {
      resultString += c;
      continue;
    }
    if(c < 0xC0 || c >= 0xF0)
      return false;
    i++;
    if (i >= utfString.Length())
      return false;
    BYTE c2 = utfString[i];
    if (c2 < 0x80)
      return false;
    c2 -= 0x80;
    if (c2 >= 0x40)
      return false;
    if (c < 0xE0)
    {
      resultString += wchar_t( ((wchar_t(c - 0xC0)) << 6) + c2);
      continue;
    }
    i++;
    if (i >= utfString.Length())
      return false;
    BYTE c3 = utfString[i];
    c3 -= 0x80;
    if (c3 >= 0x40)
      return false;
    resultString += wchar_t(((wchar_t(c - 0xE0)) << 12) + 
      ((wchar_t(c2)) << 6) + c3);
  }
  return true; 
}

void ConvertUnicodeToUTF8(const UString &unicodeString, AString &resultString)
{
  resultString.Empty();
  for(int i = 0; i < unicodeString.Length(); i++)
  {
    wchar_t c = unicodeString[i];
    if (c < 0x80)
    {
      resultString += char(c);
      continue;
    }
    if (c < 0x07FF)
    {
      resultString += char(0xC0 + (c >> 6));
      resultString += char(0x80 + (c & 0x003F));
      continue;
    }
    resultString += char(0xE0 + (c >> 12));
    resultString += char(0x80 + ((c >> 6) & 0x003F));
    resultString += char(0x80 + (c & 0x003F));
  }
}

