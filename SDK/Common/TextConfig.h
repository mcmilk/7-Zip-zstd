// Common/TextConfig.h

#pragma once

#ifndef __COMMON_TEXTCONFIG_H
#define __COMMON_TEXTCONFIG_H

#include "Common/Vector.h"
#include "Common/String.h"

struct CTextConfigPair
{
  UString ID;
  UString String;
};

bool GetTextConfig(const AString &aText, CObjectVector<CTextConfigPair> &aPairs);

int FindTextConfigItem(const CObjectVector<CTextConfigPair> &aPairs, const UString &anID);
UString GetTextConfigValue(const CObjectVector<CTextConfigPair> &aPairs, const UString &anID);

#endif


