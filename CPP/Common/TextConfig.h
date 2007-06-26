// Common/TextConfig.h

#ifndef __COMMON_TEXTCONFIG_H
#define __COMMON_TEXTCONFIG_H

#include "MyVector.h"
#include "MyString.h"

struct CTextConfigPair
{
  UString ID;
  UString String;
};

bool GetTextConfig(const AString &text, CObjectVector<CTextConfigPair> &pairs);

int FindTextConfigItem(const CObjectVector<CTextConfigPair> &pairs, const UString &id);
UString GetTextConfigValue(const CObjectVector<CTextConfigPair> &pairs, const UString &id);

#endif


