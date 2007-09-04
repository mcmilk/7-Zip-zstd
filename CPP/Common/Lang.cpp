// Common/Lang.cpp

#include "StdAfx.h"

#include "Lang.h"
#include "TextConfig.h"

#include "../Windows/FileIO.h"
#include "UTFConvert.h"
#include "Defs.h"

static bool HexStringToNumber(const UString &s, UInt32 &value)
{
  value = 0;
  if (s.IsEmpty())
    return false;
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    int a;
    if (c >= L'0' && c <= L'9')
      a = c - L'0';
    else if (c >= L'A' && c <= L'F')
      a = 10 + c - L'A';
    else if (c >= L'a' && c <= L'f')
      a = 10 + c - L'a';
    else
      return false;
    value *= 0x10;
    value += a;
  }
  return true;
}


static bool WaitNextLine(const AString &s, int &pos)
{
  for (; pos < s.Length(); pos++)
    if (s[pos] == 0x0A)
      return true;
  return false;
}

static int CompareLangItems(void *const *elem1, void *const *elem2, void *)
{
  const CLangPair &langPair1 = *(*((const CLangPair **)elem1));
  const CLangPair &langPair2 = *(*((const CLangPair **)elem2));
  return MyCompare(langPair1.Value, langPair2.Value);
}

bool CLang::Open(LPCWSTR fileName)
{
  _langPairs.Clear();
  NWindows::NFile::NIO::CInFile file;
  if (!file.Open(fileName))
    return false;
  UInt64 length;
  if (!file.GetLength(length))
    return false;
  if (length > (1 << 20))
    return false;
  AString s;
  char *p = s.GetBuffer((int)length + 1);
  UInt32 processed;
  if (!file.Read(p, (UInt32)length, processed))
    return false;
  p[(UInt32)length] = 0;
  s.ReleaseBuffer();
  file.Close();
  int pos = 0;
  if (s.Length() >= 3)
  {
    if (Byte(s[0]) == 0xEF && Byte(s[1]) == 0xBB && Byte(s[2]) == 0xBF)
      pos += 3;
  }

  /////////////////////
  // read header

  AString stringID = ";!@Lang@!UTF-8!";
  if (s.Mid(pos, stringID.Length()) != stringID)
    return false;
  pos += stringID.Length();
  
  if (!WaitNextLine(s, pos))
    return false;

  CObjectVector<CTextConfigPair> pairs;
  if (!GetTextConfig(s.Mid(pos), pairs))
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
  _langPairs.Sort(CompareLangItems, NULL);
  return true;
}

int CLang::FindItem(UInt32 value) const
{
  int left = 0, right = _langPairs.Size(); 
  while (left != right)
  {
    UInt32 mid = (left + right) / 2;
    UInt32 midValue = _langPairs[mid].Value;
    if (value == midValue)
      return mid;
    if (value < midValue)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

bool CLang::GetMessage(UInt32 value, UString &message) const
{
  int index =  FindItem(value);
  if (index < 0)
    return false;
  message = _langPairs[index].String;
  return true;
}
