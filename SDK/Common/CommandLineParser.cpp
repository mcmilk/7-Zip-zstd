// CommandLineParser.cpp

#include "StdAfx.h"

#include "CommandLineParser.h"

namespace NCommandLineParser {

static const char kSwitchID1 = '-';
static const char kSwitchID2 = '/';

static const char kSwitchMinus = '-';

static bool IsItSwitchChar(char c)
{ 
  return (c == kSwitchID1 || c == kSwitchID2); 
}

CParser::CParser(int numSwitches):
  _numSwitches(numSwitches)
{
  _switches = new CSwitchResult[_numSwitches];
}

CParser::~CParser()
{
  delete []_switches;
}

void CParser::ParseStrings(const CSwitchForm *switchForms, 
  const AStringVector &commandStrings)
{
  int numCommandStrings = commandStrings.Size();
  for (int i = 0; i < numCommandStrings; i++)
    if (!ParseString(commandStrings[i], switchForms))
      _nonSwitchStrings.Add(commandStrings[i]);
}

// if string contains switch then function updates switch structures
// out: (string is a switch)
bool CParser::ParseString(const AString &string, const CSwitchForm *switchForms)
{
  int len = string.Length();
  if (len == 0) 
    return false;
  int pos = 0;
  if (!IsItSwitchChar(string[pos]))
    return false;
  while(pos < len)
  {
    if (IsItSwitchChar(string[pos]))
      pos++;
    const int kNoLen = -1;
    int matchedSwitchIndex;
    int maxLen = kNoLen;
    for(int switchIndex = 0; switchIndex < _numSwitches; switchIndex++)
    {
      int switchLen = strlen(switchForms[switchIndex].IDString);
      if (switchLen <= maxLen || pos + switchLen > len) 
        continue;
      if(_strnicmp(switchForms[switchIndex].IDString, LPCSTR(string) + pos, switchLen) == 0)
      {
        matchedSwitchIndex = switchIndex;
        maxLen = switchLen;
      }
    }
    if (maxLen == kNoLen)
      throw "maxLen == kNoLen";
    CSwitchResult &matchedSwitch = _switches[matchedSwitchIndex];
    const CSwitchForm &switchForm = switchForms[matchedSwitchIndex];
    if ((!switchForm.Multi) && matchedSwitch.ThereIs)
      throw "switch must be single";
    matchedSwitch.ThereIs = true;
    pos += maxLen;
    int tailSize = len - pos;
    NSwitchType::EEnum type = switchForm.Type;
    switch(type)
    {
      case (NSwitchType::kPostMinus):
        {
          if (tailSize == 0)
            matchedSwitch.WithMinus = false;
          else
          {
            matchedSwitch.WithMinus = (string[pos] == kSwitchMinus);
            if (matchedSwitch.WithMinus)
              pos++;
          }
          break;
        }
      case (NSwitchType::kPostChar):
        {
          if (tailSize < switchForm.MinLen)
            throw "switch is not full";
          AString set = switchForm.PostCharSet;
          const kEmptyCharValue = -1;
          if (tailSize == 0)
            matchedSwitch.PostCharIndex = kEmptyCharValue;
          else
          {
            int index = set.Find(string[pos]);
            if (index < 0)
              matchedSwitch.PostCharIndex =  kEmptyCharValue;
            else
            {
              matchedSwitch.PostCharIndex = index;
              pos++;
            }
          }
          break;
        }
      case NSwitchType::kLimitedPostString: case NSwitchType::kUnLimitedPostString: 
        {
          int minLen = switchForm.MinLen;
          if (tailSize < minLen)
            throw "switch is not full";
          if (type == NSwitchType::kUnLimitedPostString)
          {
            matchedSwitch.PostStrings.Add(string.Mid(pos));
            return true;
          }
          int maxLen = switchForm.MaxLen;
          AString stringSwitch = string.Mid(pos, minLen);
          pos += minLen;
          for(int i = minLen; i < maxLen && pos < len; i++, pos++)
          {
            char aChar = string[pos];
            if (IsItSwitchChar(aChar))
              break;
            stringSwitch += aChar;
          }
          matchedSwitch.PostStrings.Add(stringSwitch);
        }
    }
  }
  return true;
}

const CSwitchResult& CParser::operator[](size_t index) const
{
  return _switches[index];
}

/////////////////////////////////
// Command parsing procedures

int ParseCommand(int numCommandForms, const CCommandForm *commandForms, 
    const AString &commandString, AString &postString)
{
  for(int i = 0; i < numCommandForms; i++)
  {
    const AString id = commandForms[i].IDString;
    if (commandForms[i].PostStringMode)
    {
      if(commandString.Find(id) == 0)
      {
        postString = commandString.Mid(id.Length());
        return i;
      }
    }
    else
      if (commandString == id)
      {
        postString.Empty();
        return i;
      }
  }
  return -1;
}
   
bool ParseSubCharsCommand(int numForms, const CCommandSubCharsSet *forms, 
    const CSysString &commandString, CIntVector &indices)
{
  indices.Clear();
  int numUsedChars = 0;
  for(int i = 0; i < numForms; i++)
  {
    const CCommandSubCharsSet &set = forms[i];
    int currentIndex = -1;
    int len = strlen(set.Chars);
    for(int j = 0; j < len; j++)
    {
      char c = set.Chars[j];
      int newIndex = commandString.Find(c);
      if (newIndex >= 0)
      {
        if (currentIndex >= 0)
          return false;
        if (commandString.Find(c, newIndex + 1) >= 0)
          return false;
        currentIndex = j;
        numUsedChars++;
      }
    }
    if(currentIndex == -1 && !set.EmptyAllowed)
      return false;
    indices.Add(currentIndex);
  }
  return (numUsedChars == commandString.Length());
}

}