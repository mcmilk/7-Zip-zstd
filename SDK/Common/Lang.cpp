// Common/Lang.cpp

#include "StdAfx.h"

#include "Common/Lang.h"
#include "Common/TextConfig.h"

#include "StdInStream.h"
#include "UTFConvert.h"
#include "Defs.h"

/*
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
*/
static bool HexStringToNumber(const UString &aString, UINT32 &aResultValue)
{
  aResultValue = 0;
  if (aString.IsEmpty())
    return false;
  for (int i = 0; i < aString.Length(); i++)
  {
    wchar_t aChar = aString[i];
    int a;
    if (aChar >= L'0' && aChar <= L'9')
      a = aChar - L'0';
    else if (aChar >= L'A' && aChar <= L'F')
      a = 10 + aChar - L'A';
    else if (aChar >= L'a' && aChar <= L'f')
      a = 10 + aChar - L'a';
    else
      return false;
    aResultValue *= 0x10;
    aResultValue += a;
  }
  return true;
}


static bool WaitNextLine(const AString &aString, int &aPos)
{
  for (;aPos < aString.Length(); aPos++)
    if (aString[aPos] == 0x0A)
      return true;
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

  CObjectVector<CTextConfigPair> aPairs;
  if (!GetTextConfig(aString.Mid(aPos),  aPairs))
    return false;

  m_LangPairs.Reserve(m_LangPairs.Size());
  for (int i = 0; i < aPairs.Size(); i++)
  {
    CTextConfigPair aTextConfigPair = aPairs[i];
    CLangPair aLangPair;
    if (!HexStringToNumber(aTextConfigPair.ID, aLangPair.Value))
      return false;
    aLangPair.String = aTextConfigPair.String;
    m_LangPairs.Add(aLangPair);
  }

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
