// Common/ListFileUtils.cpp

#include "StdAfx.h"

#include "ListFileUtils.h"
#include "StdInStream.h"
#include "StringConvert.h"
#include "UTFConvert.h"

/*
static const char kNewLineChar = '\n';

static const char kSpaceChar     = ' ';
static const char kTabChar       = '\t';

static const char kQuoteChar     = '\"';
static const char kEndOfLine     = '\0';

static bool IsSeparatorChar(char c)
{
  return (c == kSpaceChar || c == kTabChar);
}
*/

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
      if (!t.IsEmpty())
        resultStrings.Add(t);
      t.Empty();
    }
    else
      t += c;
  }
  t.Trim();
  if (!t.IsEmpty())
    resultStrings.Add(t);
  return true;


  /*
  AString string;
  bool quotes = false;
  int intChar;
  while((intChar = file.GetChar()) != EOF)
  {
    char c = intChar;
    if (c == kEndOfLine)
      return false;
    if (c == kNewLineChar || (IsSeparatorChar(c) && !quotes) || (c == kQuoteChar && quotes)) 
    {
      if (!string.IsEmpty())
      {
        strings.Add(string);
        string.Empty ();
        quotes = false;
      }
    }
    else if (c == kQuoteChar)
      quotes = true;
    else
      string += c;
  }
  if (!string.IsEmpty())
    if (quotes)
      return false;
    else
      strings.Add(string);
  return true;
  */
}
