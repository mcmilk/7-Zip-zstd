// StringUtils.cpp

#include "StdAfx.h"

#include "StringUtils.h"

void SplitStringToTwoStrings(const UString &src, UString &dest1, UString &dest2)
{
  dest1.Empty();
  dest2.Empty();
  bool quoteMode = false;
  int i;
  for (i = 0; i < src.Length(); i++)
  {
    wchar_t c = src[i];
    if (c == L'\"')
      quoteMode = !quoteMode;
    else if (c == L' ' && !quoteMode)
    {
      if (!quoteMode)
      {
        i++;
        break;
      }
    }
    else
      dest1 += c;
  }
  dest2 = src.Mid(i);
}

void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  UString string;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
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

