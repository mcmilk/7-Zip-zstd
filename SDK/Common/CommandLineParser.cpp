// CommandLineParser.cpp

#include "StdAfx.h"

#include "CommandLineParser.h"

namespace NComandLineParser {

static const char kSwitchID1 = '-';
static const char kSwitchID2 = '/';

static const char kSwitchMinus = '-';

static bool IsItSwitchChar(char aChar)
{ 
  return (aChar == kSwitchID1 || aChar == kSwitchID2); 
}

CParser::CParser(int aNumSwitches):
  m_NumSwitches(aNumSwitches)
{
  m_Switches = new CSwitchResult[m_NumSwitches];
}

CParser::~CParser()
{
  delete []m_Switches;
}

void CParser::ParseStrings(const CSwitchForm *aSwitchForms, 
  const CSysStringVector &aCommandStrings)
{
  int aNumCommandStrings = aCommandStrings.Size();
  for (int i = 0; i < aNumCommandStrings; i++)
    if (!ParseString(aCommandStrings[i], aSwitchForms))
      m_NonSwitchStrings.Add(aCommandStrings[i]);
}

// if aString contains switch then function updates switch structures
// out: (aString is a switch)
bool CParser::ParseString(const CSysString &aString, 
  const CSwitchForm *aSwitchForms)
{
  int aLen = aString.Length();
  if (aLen == 0) 
    return false;
  int aPos = 0;
  if (!IsItSwitchChar(aString[aPos]))
    return false;
  while(aPos < aLen)
  {
    if (IsItSwitchChar(aString[aPos]))
      aPos++;
    const int kNoLen = -1;
    int aMatchedSwitchIndex, aMaxLen = kNoLen;
    for(int aSwitch = 0; aSwitch < m_NumSwitches; aSwitch++)
    {
      int aSwitchLen = strlen(aSwitchForms[aSwitch].IDString);
      if (aSwitchLen <= aMaxLen || aPos + aSwitchLen > aLen) 
        continue;
      if(_strnicmp(aSwitchForms[aSwitch].IDString, LPCTSTR(aString) + aPos, aSwitchLen) == 0)
      {
        aMatchedSwitchIndex = aSwitch;
        aMaxLen = aSwitchLen;
      }
    }
    if (aMaxLen == kNoLen)
      throw "aMaxLen == kNoLen";
    CSwitchResult &aMatchedSwitch = m_Switches[aMatchedSwitchIndex];
    const CSwitchForm &aSwitchForm = aSwitchForms[aMatchedSwitchIndex];
    if ((!aSwitchForm.Multi) && aMatchedSwitch.ThereIs)
      throw "switch must be single";
    aMatchedSwitch.ThereIs = true;
    aPos += aMaxLen;
    int aTailSize = aLen - aPos;
    NSwitchType::EEnum aType = aSwitchForm.Type;
    switch(aType)
    {
      case (NSwitchType::kPostMinus):
        {
          if (aTailSize == 0)
            aMatchedSwitch.WithMinus = false;
          else
          {
            aMatchedSwitch.WithMinus = (aString[aPos] == kSwitchMinus);
            if (aMatchedSwitch.WithMinus)
              aPos++;
          }
          break;
        }
      case (NSwitchType::kPostChar):
        {
          if (aTailSize < aSwitchForm.MinLen)
            throw "switch is not full";
          CSysString aSet = aSwitchForm.PostCharSet;
          const kEmptyCharValue = -1;
          if (aTailSize == 0)
            aMatchedSwitch.PostCharIndex = kEmptyCharValue;
          else
          {
            int anIndex = aSet.Find(aString[aPos]);
            if (anIndex < 0)
              aMatchedSwitch.PostCharIndex =  kEmptyCharValue;
            else
            {
              aMatchedSwitch.PostCharIndex = anIndex;
              aPos++;
            }
          }
          break;
        }
      case NSwitchType::kLimitedPostString: case NSwitchType::kUnLimitedPostString: 
        {
          int aMinLen = aSwitchForm.MinLen;
          if (aTailSize < aMinLen)
            throw "switch is not full";
          if (aType == NSwitchType::kUnLimitedPostString)
          {
            aMatchedSwitch.PostStrings.Add(aString.Mid(aPos));
            return true;
          }
          int aMaxLen = aSwitchForm.MaxLen;
          CSysString aStringSwitch = aString.Mid(aPos, aMinLen);
          aPos += aMinLen;
          for(int i = aMinLen; i < aMaxLen && aPos < aLen; i++, aPos++)
          {
            char aChar = aString[aPos];
            if (IsItSwitchChar(aChar))
              break;
            aStringSwitch += aChar;
          }
          aMatchedSwitch.PostStrings.Add(aStringSwitch);
        }
    }
  }
  return true;
}

const CSwitchResult& CParser::operator[](size_t anIndex) const
{
  return m_Switches[anIndex];
}

/////////////////////////////////
// Command parsing procedures

int ParseCommand(int aNumCommandForms, const CCommandForm *aCommandForms, 
    const CSysString &aCommandString, CSysString &aPostString)
{
  for(int i = 0; i < aNumCommandForms; i++)
  {
    const CSysString anID = aCommandForms[i].IDString;
    if (aCommandForms[i].PostStringMode)
    {
      if(aCommandString.Find(anID) == 0)
      {
        aPostString = aCommandString.Mid(anID.Length());
        return i;
      }
    }
    else
      if (aCommandString == anID)
      {
        aPostString.Empty();
        return i;
      }
  }
  return -1;
}
   
bool ParseSubCharsCommand(int aNumForms, const CCommandSubCharsSet *aForms, 
    const CSysString &aCommandString, CIntVector &anIndexes)
{
  anIndexes.Clear();
  int aNumUsedChars = 0;
  for(int i = 0; i < aNumForms; i++)
  {
    const CCommandSubCharsSet &aSet = aForms[i];
    int aCurrentIndex = -1;
    int aLen = strlen(aSet.Chars);
    for(int j = 0; j < aLen; j++)
    {
      char aChar = aSet.Chars[j];
      int aNewIndex = aCommandString.Find(aChar);
      if (aNewIndex >= 0)
      {
        if (aCurrentIndex >= 0)
          return false;
        if (aCommandString.Find(aChar, aNewIndex + 1) >= 0)
          return false;
        aCurrentIndex = j;
        aNumUsedChars++;
      }
    }
    if(aCurrentIndex == -1 && !aSet.EmptyAllowed)
      return false;
    anIndexes.Add(aCurrentIndex);
  }
  return (aNumUsedChars == aCommandString.Length());
}

}