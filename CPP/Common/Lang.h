// Common/Lang.h

#ifndef __COMMON_LANG_H
#define __COMMON_LANG_H

#include "MyString.h"
#include "Types.h"

struct CLangPair
{
  UInt32 Value;
  UString String;
};

class CLang
{
  CObjectVector<CLangPair> _langPairs;
public:
  bool Open(CFSTR fileName);
  void Clear() { _langPairs.Clear(); }
  int FindItem(UInt32 value) const;
  bool GetMessage(UInt32 value, UString &message) const;
};

#endif


