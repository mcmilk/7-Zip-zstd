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
  CObjectVector<CLangPair> m_LangPairs;
public:
  bool Open(LPCTSTR aFileName);
  int FindItem(UINT32 aValue) const;
  bool GetMessage(UINT32 aValue, UString &aMessage) const;
};

#endif


