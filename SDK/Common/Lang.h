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
  UString m_Name;
  CObjectVector<CLangPair> m_LangPairs;
public:
  const UString &GetName() const { return m_Name; }
  bool Open(LPCTSTR aFileName);
  const UString &GetMessage(UINT32 Value) const;
  int FindItem(UINT32 aValue) const;
  bool GetMessage(UINT32 aValue, UString &aMessage) const;
};

#endif


