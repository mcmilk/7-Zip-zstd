// Common/TextPairs.cpp

#include "StdAfx.h"

#include "TextPairs.h"

#include "Common/Defs.h"
#include "Common/UTFConvert.h"

static const wchar_t kNewLineChar = '\n';

static const wchar_t kSpaceChar     = ' ';
static const wchar_t kTabChar       = '\t';

static const wchar_t kQuoteChar     = '\"';
static const wchar_t kEndOfLine     = '\0';

static const wchar_t kBOM = wchar_t(0xFEFF);

static bool IsSeparatorChar(wchar_t c)
{
  return (c == kSpaceChar || c == kTabChar);
}

static UString GetIDString(const wchar_t *srcString, int &finishPos)
{
  UString result;
  bool quotes = false;
  for (finishPos = 0;;)
  {
    wchar_t c = srcString[finishPos];
    if (c == kEndOfLine)
      break;
    finishPos++;
    bool isSeparatorChar = IsSeparatorChar(c);
    if (c == kNewLineChar || (isSeparatorChar && !quotes) 
        || (c == kQuoteChar && quotes)) 
      break;
    else if (c == kQuoteChar)
      quotes = true;
    else
      result += c;
  }
  result.Trim();
  return result;
}

static UString GetValueString(const wchar_t *srcString, int &finishPos)
{
  UString result;
  for (finishPos = 0;;)
  {
    wchar_t c = srcString[finishPos];
    if (c == kEndOfLine)
      break;
    finishPos++;
    if (c == kNewLineChar) 
      break;
    result += c;
  }
  result.Trim();
  return result;
}

static bool GetTextPairs(const UString &srcString, CObjectVector<CTextPair> &pairs)
{
  pairs.Clear();
  UString srcString2 = srcString;
  srcString2.Replace(L"\x0D", L"");

  pairs.Clear();
  int pos = 0;
  
  /////////////////////
  // read strings

  if (srcString2.Length() > 0)
  {
    if (srcString2[0] == kBOM)
      pos++;
  }
  while (pos < srcString2.Length())
  {
    int finishPos;
    UString id = GetIDString((const wchar_t *)srcString2 + pos, finishPos);
    pos += finishPos;
    if (id.IsEmpty())
      continue;
    UString value = GetValueString((const wchar_t *)srcString2 + pos, finishPos);
    pos += finishPos;
    if (!id.IsEmpty())
    {
      CTextPair pair;
      pair.ID = id;
      pair.Value = value;
      pairs.Add(pair);
    }
  }
  return true;
}

int FindItem(const CObjectVector<CTextPair> &pairs, const UString &id)
{
  for (int  i = 0; i < pairs.Size(); i++)
    if (pairs[i].ID.CompareNoCase(id) == 0)
      return i;
  return -1;
}

UString GetTextConfigValue(const CObjectVector<CTextPair> &pairs, const UString &id)
{
  int index = FindItem(pairs, id);
  if (index < 0)
    return UString();
  return pairs[index].Value;
}

static int ComparePairIDs(const UString &s1, const UString &s2)
  { return s1.CollateNoCase(s2); }
static int ComparePairItems(const CTextPair &p1, const CTextPair &p2)
  { return ComparePairIDs(p1.ID, p2.ID); }
static int ComparePairItems(const void *a1, const void *a2)
{   
  return ComparePairItems(
      *(*((const CTextPair **)a1)),
      *(*((const CTextPair **)a2)));
}

void CPairsStorage::Sort()
{
  CPointerVector &pointerVector = Pairs;
  qsort(&pointerVector[0], Pairs.Size(), sizeof(void *), 
      ComparePairItems);
}

int CPairsStorage::FindID(const UString &id, int &insertPos)
{
  int left = 0, right = Pairs.Size(); 
  while (left != right)
  {
    UINT32 mid = (left + right) / 2;
    int compResult = ComparePairIDs(id, Pairs[mid].ID);
    if (compResult == 0)
      return mid;
    if (compResult < 0)
      right = mid;
    else
      left = mid + 1;
  }
  insertPos = left;
  return -1;
}

int CPairsStorage::FindID(const UString &id)
{
  int pos;
  return FindID(id, pos);
}

void CPairsStorage::AddPair(const CTextPair &pair)
{
  int insertPos;
  int pos = FindID(pair.ID, insertPos);
  if (pos >= 0)
    Pairs[pos] = pair;
  else
    Pairs.Insert(insertPos, pair);
}

void CPairsStorage::DeletePair(const UString &id)
{
  int pos = FindID(id);
  if (pos >= 0)
    Pairs.Delete(pos);
}

bool CPairsStorage::GetValue(const UString &id, UString &value)
{
  value.Empty();
  int pos = FindID(id);
  if (pos < 0)
    return false;
  value = Pairs[pos].Value;
  return true;
}

UString CPairsStorage::GetValue(const UString &id)
{
  int pos = FindID(id);
  if (pos < 0)
    return UString();
  return Pairs[pos].Value;
}

bool CPairsStorage::ReadFromString(const UString &text)
{
  bool result = ::GetTextPairs(text, Pairs);
  if (result)
    Sort();
  else
    Pairs.Clear();
  return  result;
}

void CPairsStorage::SaveToString(UString &text)
{
  for (int i = 0; i < Pairs.Size(); i++)
  {
    const CTextPair &pair = Pairs[i];
    bool multiWord = (pair.ID.Find(L' ') >= 0);
    if (multiWord)
      text += L'\"';
    text += pair.ID;
    if (multiWord)
      text += L'\"';
    text += L' ';
    text += pair.Value;
    text += L'\n';
  }
}
