// Common/ListFileUtils.cpp

#include "StdAfx.h"

#include "ListFileUtils.h"
#include "StdInStream.h"
#include "StringConvert.h"
#include "UTFConvert.h"

static const char kQuoteChar     = '\"';
static void RemoveQuote(UString &s)
{
  if (s.Length() >= 2)
    if (s[0] == kQuoteChar && s[s.Length() - 1] == kQuoteChar)
      s = s.Mid(1, s.Length() - 2);
}

bool ReadNamesFromListFile(LPCTSTR fileName, UStringVector &resultStrings, 
    UINT codePage)
{
  CStdInStream file;
  if (!file.Open(fileName))
    return false;

  AString s;
  file.ReadToString(s);
  UString u;
  if (codePage == CP_UTF8)
  {
    if (!ConvertUTF8ToUnicode(s, u))
      return false;
  }
  else
    u = MultiByteToUnicodeString(s, codePage);
  UString t;
  for(int i = 0; i < u.Length(); i++)
  {
    wchar_t c = u[i];
    if (c == L'\n')
    {
      t.Trim();
      RemoveQuote(t);
      if (!t.IsEmpty())
        resultStrings.Add(t);
      t.Empty();
    }
    else
      t += c;
  }
  t.Trim();
  RemoveQuote(t);
  if (!t.IsEmpty())
    resultStrings.Add(t);
  return true;
}
