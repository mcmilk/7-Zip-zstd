// Common/ListFileUtils.cpp

#include "StdAfx.h"

#include "ListFileUtils.h"
#include "StdInStream.h"

static const char kNewLineChar = '\n';

static const char kSpaceChar     = ' ';
static const char kTabChar       = '\t';

static const char kQuoteChar     = '\"';
static const char kEndOfLine     = '\0';

static bool IsSeparatorChar(char aChar)
{
  return (aChar == kSpaceChar || aChar == kTabChar);
}

bool ReadNamesFromListFile(LPCTSTR aFileName, AStringVector &aStrings)
{
  CStdInStream aFile;
  if (!aFile.Open(aFileName))
    return false;
  AString aString;
  bool aQuotes = false;
  int aIntChar;
  while((aIntChar = aFile.GetChar()) != EOF)
  {
    char c = aIntChar;
    if (c == kEndOfLine)
      return false;
    if (c == kNewLineChar || (IsSeparatorChar(c) && !aQuotes) || (c == kQuoteChar && aQuotes)) 
    {
      if (!aString.IsEmpty())
      {
        aStrings.Add(aString);
        aString.Empty ();
        aQuotes = false;
      }
    }
    else if (c == kQuoteChar)
      aQuotes = true;
    else
      aString += c;
  }
  if (!aString.IsEmpty())
    if (aQuotes)
      return false;
    else
      aStrings.Add(aString);
  return true;
}
