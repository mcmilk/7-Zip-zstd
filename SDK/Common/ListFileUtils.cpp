// Common/ListFileUtils.cpp

#include "StdAfx.h"

#include "ListFileUtils.h"
#include "StdInStream.h"

static const char kNewLineChar = '\n';

static const char kSpaceChar     = ' ';
static const char kTabChar       = '\t';

static const char kQuoteChar     = '\"';
static const char kEndOfLine     = '\0';

static bool IsSeparatorChar(char c)
{
  return (c == kSpaceChar || c == kTabChar);
}

bool ReadNamesFromListFile(LPCTSTR fileName, AStringVector &strings)
{
  CStdInStream file;
  if (!file.Open(fileName))
    return false;
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
}
