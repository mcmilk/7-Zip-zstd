// Common/Lang.cpp

#include "StdAfx.h"

#include "Common/Lang.h"

#include "StdInStream.h"
#include "UTFConvert.h"
#include "Defs.h"

static UINT32 HexStringToNumber(const char *aString, int &aFinishPos)
{
  UINT32 aNumber = 0;
  for (aFinishPos = 0; aFinishPos < 8; aFinishPos++)
  {
    char aChar = aString[aFinishPos];
    int a;
    if (aChar >= '0' && aChar <= '9')
      a = aChar - '0';
    else if (aChar >= 'A' && aChar <= 'F')
      a = 10 + aChar - 'A';
    else if (aChar >= 'a' && aChar <= 'f')
      a = 10 + aChar - 'a';
    else
      return aNumber;
    aNumber *= 0x10;
    aNumber += a;
  }
  return aNumber;
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
    if (aChar != ' ' && aChar != 0x0A && aChar != 0x0D)
    {
      if (aChar != ';')
        return true;
      if (!WaitNextLine(aString, aPos))
        return false;
    }
  }
  return false;
}

static int CompareLangItems( const void *anElem1, const void *anElem2)
{
  const CLangPair &aLangPair1 = *(*((const CLangPair **)anElem1));
  const CLangPair &aLangPair2 = *(*((const CLangPair **)anElem2));
  return MyCompare(aLangPair1.Value, aLangPair2.Value);
}

bool CLang::Open(LPCTSTR aFileName)
{
  m_LangPairs.Clear();
  CStdInStream aFile;
  if (!aFile.Open(aFileName))
    return false;
  AString aString;
  aFile.ReadToString(aString);
  aFile.Close();
  int aPos = 0;
  if (aString.Length() >= 3)
  {
    if (BYTE(aString[0]) == 0xEF && BYTE(aString[1]) == 0xBB && BYTE(aString[2]) == 0xBF)
      aPos += 3;
  }

  /////////////////////
  // read header

  AString aStringID = ";!@Lang@!UTF-8!";
  if (aString.Mid(aPos, aStringID.Length()) != aStringID)
    return false;
  aPos += aStringID.Length();
  
  if (!WaitNextLine(aString, aPos))
    return false;

  /////////////////////
  // read strings

  while (true)
  {
    if (!SkipSpaces(aString, aPos))
      break;
    CLangPair aLangPair;
    int aFinishPos;
    aLangPair.Value = HexStringToNumber(((const char *)aString) + aPos, aFinishPos);
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
    if (!ConvertUTF8ToUnicode(aMessage, aLangPair.String))
      return false;
    m_LangPairs.Add(aLangPair);
  }

  /*
  CRecordVector<int> anIndexes;
  anIndexes.Reserve(aLangPairs.Size());
  m_LangPairs.Reserve(aLangPairs.Size());
  for (int i = 0; i < aLangPairs.Size(); i++)
    anIndexes.Add(i);
    */
  CPointerVector &aPointerVector = m_LangPairs;
  qsort(&aPointerVector[0], m_LangPairs.Size(), sizeof(void *), CompareLangItems);
  return true;
}

int CLang::FindItem(UINT32 aValue) const
{
  int aLeft = 0, aRight = m_LangPairs.Size(); 
  while (aLeft != aRight)
  {
    int aMid = (aLeft + aRight) / 2;
    int aMidValue = m_LangPairs[aMid].Value;
    if (aValue == aMidValue)
      return aMid;
    if (aValue < aMidValue)
      aRight = aMid;
    else
      aLeft = aMid + 1;
  }
  return -1;
}

bool CLang::GetMessage(UINT32 aValue, UString &aMessage) const
{
  int aIndex =  FindItem(aValue);
  if (aIndex < 0)
    return false;
  aMessage = m_LangPairs[aIndex].String;
  return true;
}
