// UTFConvert.cpp

#include "StdAfx.h"

#include "UTFConvert.h"

bool ConvertUTF8ToUnicode(const AString &anUTFString, UString &anResultString)
{
  anResultString.Empty();
  for(int i = 0; i < anUTFString.Length(); i++)
  {
    BYTE aChar = anUTFString[i];
    if (aChar < 0x80)
    {
      anResultString += aChar;
      continue;
    }
    if(aChar < 0xC0 || aChar >= 0xF0)
      return false;
    i++;
    if (i >= anUTFString.Length())
      return false;
    BYTE aChar2 = anUTFString[i];
    if (aChar2 < 0x80)
      return false;
    aChar2 -= 0x80;
    if (aChar2 >= 0x40)
      return false;
    if (aChar < 0xE0)
    {
      anResultString += wchar_t( ((wchar_t(aChar - 0xC0)) << 6) + aChar2);
      continue;
    }
    i++;
    if (i >= anUTFString.Length())
      return false;
    BYTE aChar3 = anUTFString[i];
    aChar3 -= 0x80;
    if (aChar3 >= 0x40)
      return false;
    anResultString += wchar_t(((wchar_t(aChar - 0xE0)) << 12) + 
      ((wchar_t(aChar2)) << 6) + aChar3);
  }
  return true; 
}

void ConvertUnicodeToUTF8(const UString &anUnicodeString, AString &anResultString)
{
  anResultString.Empty();
  for(int i = 0; i < anUnicodeString.Length(); i++)
  {
    wchar_t aChar = anUnicodeString[i];
    if (aChar < 0x80)
    {
      anResultString += char(aChar);
      continue;
    }
    if (aChar < 0x07FF)
    {
      anResultString += char(0xC0 + (aChar >> 6));
      anResultString += char(0x80 + (aChar & 0x003F));
      continue;
    }
    anResultString += char(0xE0 + (aChar >> 12));
    anResultString += char(0x80 + ((aChar >> 6) & 0x003F));
    anResultString += char(0x80 + (aChar & 0x003F));
  }
}

