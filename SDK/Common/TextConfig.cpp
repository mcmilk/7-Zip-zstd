// Common/TextConfig.cpp

#include "StdAfx.h"

#include "Common/TextConfig.h"

#include "Defs.h"
#include "Common/UTFConvert.h"

static bool IsDelimitChar(char aChar)
{
  return (aChar == ' ' || aChar == 0x0A || aChar == 0x0D ||
      aChar == '\0' || aChar == '\t');
}
    
static AString GetIDString(const char *aString, int &aFinishPos)
{
  AString aResult;
  for (aFinishPos = 0; true; aFinishPos++)
  {
    char aChar = aString[aFinishPos];
    if (IsDelimitChar(aChar) || aChar == '=')
      return aResult;
    aResult += aChar;
  }
}

static bool WaitNextLine(const AString &aString, int &aPos)
{
  for (;aPos < aString.Length(); aPos++)
    if (aString[aPos] == 0x0A)
      return true;
  return false;
}

static bool SkipSpaces(const AString &aString, int &aPos)
{
  for (;aPos < aString.Length(); aPos++)
  {
    char aChar = aString[aPos];
    if (!IsDelimitChar(aChar))
    {
      if (aChar != ';')
        return true;
      if (!WaitNextLine(aString, aPos))
        return false;
    }
  }
  return false;
}

bool GetTextConfig(const AString &aString, CObjectVector<CTextConfigPair> &aPairs)
{
  aPairs.Clear();
  int aPos = 0;

  /////////////////////
  // read strings

  while (true)
  {
    if (!SkipSpaces(aString, aPos))
      break;
    CTextConfigPair aPair;
    int aFinishPos;
    AString aTemp = GetIDString(((const char *)aString) + aPos, aFinishPos);
    if (!ConvertUTF8ToUnicode(aTemp, aPair.ID))
      return false;
    if (aFinishPos == 0)
      return false;
    aPos += aFinishPos;
    if (!SkipSpaces(aString, aPos))
      return false;
    if (aString[aPos] != '=')
      return false;
    aPos++;
    if (!SkipSpaces(aString, aPos))
      return false;
    if (aString[aPos] != '\"')
      return false;
    aPos++;
    AString aMessage;
    while(true)
    {
      if (aPos >= aString.Length())
        return false;
      char aChar = aString[aPos++];
      if (aChar == '\"')
        break;
      if (aChar == '\\')
      {
        char aChar = aString[aPos++];
        switch(aChar)
        {
          case 'n':
            aMessage += '\n';
            break;
          case 't':
            aMessage += '\t';
            break;
          case '\\':
            aMessage += '\\';
            break;
          case '\"':
            aMessage += '\"';
            break;
          default:
            aMessage += '\\';
            aMessage += aChar;
            break;
        }
      }
      else
        aMessage += aChar;
    }
    if (!ConvertUTF8ToUnicode(aMessage, aPair.String))
      return false;
    aPairs.Add(aPair);
  }
  return true;
}

int FindItem(const CObjectVector<CTextConfigPair> &aPairs, const UString &anID)
{
  for (int  i = 0; i < aPairs.Size(); i++)
    if (aPairs[i].ID.Compare(anID) == 0)
      return i;
  return -1;
}

UString GetTextConfigValue(const CObjectVector<CTextConfigPair> &aPairs, const UString &anID)
{
  int anIndex = FindItem(aPairs, anID);
  if (anIndex < 0)
    return UString();
  return aPairs[anIndex].String;
}
