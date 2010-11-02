// PropVariantUtils.cpp

#include "StdAfx.h"

#include "PropVariantUtils.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"

using namespace NWindows;

static AString GetHex(UInt32 v)
{
  char sz[32] = { '0', 'x' };
  ConvertUInt64ToString(v, sz + 2, 16);
  return sz;
}

void StringToProp(const AString &s, NCOM::CPropVariant &prop)
{
  prop = MultiByteToUnicodeString(s);
}

void PairToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 value, NCOM::CPropVariant &prop)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    if (p.Value == value)
      s = p.Name;
  }
  if (s.IsEmpty())
    s = GetHex(value);
  StringToProp(s, prop);
}

AString TypeToString(const char *table[], unsigned num, UInt32 value)
{
  if (value < num)
    return table[value];
  return GetHex(value);
}

void TypeToProp(const char *table[], unsigned num, UInt32 value, NCOM::CPropVariant &prop)
{
  StringToProp(TypeToString(table, num, value), prop);
}


AString FlagsToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags)
{
  AString s;
  for (unsigned i = 0; i < num; i++)
  {
    const CUInt32PCharPair &p = pairs[i];
    UInt32 flag = (UInt32)1 << (unsigned)p.Value;
    if ((flags & flag) != 0)
    {
      if (!s.IsEmpty())
        s += ' ';
      s += p.Name;
    }
    flags &= ~flag;
  }
  if (flags != 0)
  {
    if (!s.IsEmpty())
      s += ' ';
    s += GetHex(flags);
  }
  return s;
}

void FlagsToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags, NCOM::CPropVariant &prop)
{
  StringToProp(FlagsToString(pairs, num, flags), prop);
}

