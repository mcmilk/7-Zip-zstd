// StringUtils.cpp

#include "StdAfx.h"

#include "StringUtils.h"

void SplitStringToTwoStrings(const UString &src, UString &dest1, UString &dest2)
{
  dest1.Empty();
  dest2.Empty();
  bool aQuoteMode = false;
  for (int i = 0; i < src.Length(); i++)
  {
    wchar_t aChar = src[i];
    if (aChar == L'\"')
      aQuoteMode = !aQuoteMode;
    else if (aChar == L' ' && !aQuoteMode)
    {
      if (!aQuoteMode)
      {
        i++;
        break;
      }
    }
    else 
      dest1 += aChar;
  }
  dest2 = src.Mid(i);
}

void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  UString string;
  int aLen = srcString.Length();
  if (aLen == 0)
    return;
  for (int i = 0; i < aLen; i++)
  {
    wchar_t c = srcString[i];
    if (c == L' ')
    {
      if (!string.IsEmpty())
      {
        destStrings.Add(string);
        string.Empty();
      }
    }
    else
      string += c;
  }
  if (!string.IsEmpty())
    destStrings.Add(string);
}

UString JoinStrings(const UStringVector &srcStrings)
{
  UString destString;
  for (int i = 0; i < srcStrings.Size(); i++)
  {
    if (i != 0)
      destString += L' ';
    destString += srcStrings[i];
  }
  return destString;
}

/*
void SplitString(const CSysString &srcString, CSysStringVector &destStrings)
{
  destStrings.Clear();
  UStringVector destStringsTemp;
  SplitString(GetUnicodeString(srcString), destStringsTemp);
  for (int i = 0; i < destStringsTemp.Size(); i++);
    destStrings.Add(GetSysUnicodeString
}
*/
