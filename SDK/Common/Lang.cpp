// Common/Lang.cpp

#include "StdAfx.h"

#include "Common/Lang.h"
#include "Common/TextConfig.h"

#include "StdInStream.h"
#include "UTFConvert.h"
#include "Defs.h"

/*
static UINT32 HexStringToNumber(const char *string, int &finishPos)
{
  UINT32 number = 0;
  for (finishPos = 0; finishPos < 8; finishPos++)
  {
    char c = string[finishPos];
    int a;
    if (c >= '0' && c <= '9')
      a = c - '0';
    else if (c >= 'A' && c <= 'F')
      a = 10 + c - 'A';
    else if (c >= 'a' && c <= 'f')
      a = 10 + c - 'a';
    else
      return number;
    number *= 0x10;
    number += a;
  }
  return number;
}
*/
static bool HexStringToNumber(const UString &string, UINT32 &aResultValue)
{
  aResultValue = 0;
  if (string.IsEmpty())
    return false;
  for (int i = 0; i < string.Length(); i++)
  {
    wchar_t c = string[i];
    int a;
    if (c >= L'0' && c <= L'9')
      a = c - L'0';
    else if (c >= L'A' && c <= L'F')
      a = 10 + c - L'A';
    else if (c >= L'a' && c <= L'f')
      a = 10 + c - L'a';
    else
      return false;
    aResultValue *= 0x10;
    aResultValue += a;
  }
  return true;
}


static bool WaitNextLine(const AString &string, int &pos)
{
  for (;pos < string.Length(); pos++)
    if (string[pos] == 0x0A)
      return true;
  return false;
}

static int __cdecl CompareLangItems( const void *elem1, const void *elem2)
{
  const CLangPair &langPair1 = *(*((const CLangPair **)elem1));
  const CLangPair &langPair2 = *(*((const CLangPair **)elem2));
  return MyCompare(langPair1.Value, langPair2.Value);
}

bool CLang::Open(LPCTSTR fileName)
{
  _langPairs.Clear();
  CStdInStream file;
  if (!file.Open(fileName))
    return false;
  AString string;
  file.ReadToString(string);
  file.Close();
  int pos = 0;
  if (string.Length() >= 3)
  {
    if (BYTE(string[0]) == 0xEF && BYTE(string[1]) == 0xBB && BYTE(string[2]) == 0xBF)
      pos += 3;
  }

  /////////////////////
  // read header

  AString stringID = ";!@Lang@!UTF-8!";
  if (string.Mid(pos, stringID.Length()) != stringID)
    return false;
  pos += stringID.Length();
  
  if (!WaitNextLine(string, pos))
    return false;

  CObjectVector<CTextConfigPair> pairs;
  if (!GetTextConfig(string.Mid(pos),  pairs))
    return false;

  _langPairs.Reserve(_langPairs.Size());
  for (int i = 0; i < pairs.Size(); i++)
  {
    CTextConfigPair textConfigPair = pairs[i];
    CLangPair langPair;
    if (!HexStringToNumber(textConfigPair.ID, langPair.Value))
      return false;
    langPair.String = textConfigPair.String;
    _langPairs.Add(langPair);
  }

  CPointerVector &pointerVector = _langPairs;
  qsort(&pointerVector[0], _langPairs.Size(), sizeof(void *), CompareLangItems);
  return true;
}

int CLang::FindItem(UINT32 value) const
{
  int left = 0, right = _langPairs.Size(); 
  while (left != right)
  {
    UINT32 mid = (left + right) / 2;
    UINT32 midValue = _langPairs[mid].Value;
    if (value == midValue)
      return mid;
    if (value < midValue)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

bool CLang::GetMessage(UINT32 value, UString &message) const
{
  int index =  FindItem(value);
  if (index < 0)
    return false;
  message = _langPairs[index].String;
  return true;
}
