// Common/Lang.h

#pragma once

#ifndef __COMMON_LANG_H
#define __COMMON_LANG_H

#include "Common/Vector.h"
#include "Common/String.h"

struct CLangPair
{
  UINT32 Value;
  UString String;
};

class CLang
{
  CObjectVector<CLangPair> _langPairs;
public:
  bool Open(LPCTSTR fileName);
  void Clear() { _langPairs.Clear(); }
  int FindItem(UINT32 value) const;
  bool GetMessage(UINT32 value, UString &message) const;
};

#endif


