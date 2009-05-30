// TextPairs.h

#ifndef __FM_TEXT_PAIRS_H
#define __FM_TEXT_PAIRS_H

#include "Common/MyString.h"

struct CTextPair
{
  UString ID;
  UString Value;
};

class CPairsStorage
{
  CObjectVector<CTextPair> Pairs;
  int FindID(const UString &id, int &insertPos);
  int FindID(const UString &id);
  void Sort();
public:
  void Clear() { Pairs.Clear(); };
  bool ReadFromString(const UString &text);
  void SaveToString(UString &text);

  bool GetValue(const UString &id, UString &value);
  UString GetValue(const UString &id);
  void AddPair(const CTextPair &pair);
  void DeletePair(const UString &id);
};

#endif
