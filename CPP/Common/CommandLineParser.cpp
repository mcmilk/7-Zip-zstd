// CommandLineParser.cpp

#include "StdAfx.h"

#include "CommandLineParser.h"

namespace NCommandLineParser {

bool SplitCommandLine(const UString &src, UString &dest1, UString &dest2)
{
  unsigned qcount = 0, bcount = 0;
  wchar_t c; const wchar_t *s = src.Ptr();

  dest1.Empty();

  while ((c = *s++) != 0)
  {
    switch (c)
    {
      case L'\\':
        // a backslash - firstly add it as a regular char, 
        // we'll delete escaped later (if followed by quote):
        dest1 += c;
        bcount++;
      break;
      case L'"':
        // check quote char is escaped:
        if (!(bcount & 1))
        {
          // preceded by an even number of '\', this is half that
          // number of '\' (delete bcount/2 chars):
          dest1.DeleteFrom(dest1.Len() - bcount/2);
          // count quote chars:
          qcount++;
        }
        else
        {
          // preceded by an odd number of '\', this is half that
          // number of '\' followed by an escaped '"':
          dest1.DeleteFrom(dest1.Len() - bcount/2 - 1);
          dest1 += L'"';
        }
        bcount = 0;
        // now count the number of consecutive quotes (inclusive
        // the quote that lead us here):
        while (*s == L'"')
        {
          s++;
          if (++qcount == 3)
          {
            dest1 += L'"';
            qcount = 0;
          }
        }
        if (qcount == 2)
          qcount = 0;
      break;
      case L' ':
      case L'\t':
        // a space (end of arg or regular char):
        if (!qcount)
        {
          // end of argument:
          bcount = 0;
          // skip to the next one:
          while (isblank(*s)) { s++; };
          goto done;
        }
      // no break - a space as regular char:
      default:
        // a regular character
        dest1 += c;
        bcount = 0;
    }
  }
  s--; // back to NTS-zero char
done:
  dest2 = s;
  return (dest1.Len() || src[0]);
}

void SplitCommandLine(const UString &s, UStringVector &parts)
{
  UString sTemp (s);
  sTemp.Trim();
  parts.Clear();
  for (;;)
  {
    UString s1, s2;
    if (SplitCommandLine(sTemp, s1, s2))
      parts.Add(s1);
    if (s2.IsEmpty())
      break;
    sTemp = s2;
  }
}


static const char * const kStopSwitchParsing = "--";

static bool inline IsItSwitchChar(wchar_t c)
{
  return (c == '-');
}

CParser::CParser():
  _switches(NULL),
  StopSwitchIndex(-1)
{
}

CParser::~CParser()
{
  delete []_switches;
}


// if (s) contains switch then function updates switch structures
// out: true, if (s) is a switch
bool CParser::ParseString(const UString &s, const CSwitchForm *switchForms, unsigned numSwitches)
{
  if (s.IsEmpty() || !IsItSwitchChar(s[0]))
    return false;

  unsigned pos = 1;
  unsigned switchIndex = 0;
  int maxLen = -1;
  
  for (unsigned i = 0; i < numSwitches; i++)
  {
    const char * const key = switchForms[i].Key;
    unsigned switchLen = MyStringLen(key);
    if ((int)switchLen <= maxLen || pos + switchLen > s.Len())
      continue;
    if (IsString1PrefixedByString2_NoCase_Ascii((const wchar_t *)s + pos, key))
    {
      switchIndex = i;
      maxLen = (int)switchLen;
    }
  }

  if (maxLen < 0)
  {
    ErrorMessage = "Unknown switch:";
    return false;
  }

  pos += (unsigned)maxLen;
  
  CSwitchResult &sw = _switches[switchIndex];
  const CSwitchForm &form = switchForms[switchIndex];
  
  if (!form.Multi && sw.ThereIs)
  {
    ErrorMessage = "Multiple instances for switch:";
    return false;
  }

  sw.ThereIs = true;

  const unsigned rem = s.Len() - pos;
  if (rem < form.MinLen)
  {
    ErrorMessage = "Too short switch:";
    return false;
  }
  
  sw.WithMinus = false;
  sw.PostCharIndex = -1;
  
  switch (form.Type)
  {
    case NSwitchType::kMinus:
      if (rem == 1)
      {
        sw.WithMinus = (s[pos] == '-');
        if (sw.WithMinus)
          return true;
        ErrorMessage = "Incorrect switch postfix:";
        return false;
      }
      break;
      
    case NSwitchType::kChar:
      if (rem == 1)
      {
        wchar_t c = s[pos];
        if (c <= 0x7F)
        {
          sw.PostCharIndex = FindCharPosInString(form.PostCharSet, (char)c);
          if (sw.PostCharIndex >= 0)
            return true;
        }
        ErrorMessage = "Incorrect switch postfix:";
        return false;
      }
      break;
      
    case NSwitchType::kString:
    {
      sw.PostStrings.Add(s.Ptr(pos));
      return true;
    }
  }

  if (pos != s.Len())
  {
    ErrorMessage = "Too long switch:";
    return false;
  }
  return true;
}


bool CParser::ParseStrings(const CSwitchForm *switchForms, unsigned numSwitches, const UStringVector &commandStrings)
{
  StopSwitchIndex = -1;
  ErrorMessage.Empty();
  ErrorLine.Empty();
  NonSwitchStrings.Clear();
  delete []_switches;
  _switches = NULL;
  _switches = new CSwitchResult[numSwitches];
  
  FOR_VECTOR (i, commandStrings)
  {
    const UString &s = commandStrings[i];
    if (StopSwitchIndex < 0)
    {
      if (s.IsEqualTo(kStopSwitchParsing))
      {
        StopSwitchIndex = (int)NonSwitchStrings.Size();
        continue;
      }
      if (!s.IsEmpty() && IsItSwitchChar(s[0]))
      {
        if (ParseString(s, switchForms, numSwitches))
          continue;
        ErrorLine = s;
        return false;
      }
    }
    NonSwitchStrings.Add(s);
  }
  return true;
}

}
