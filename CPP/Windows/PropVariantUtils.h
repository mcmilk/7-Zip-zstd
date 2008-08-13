// Windows/PropVariantUtils.h

#ifndef __PROP_VARIANT_UTILS_H
#define __PROP_VARIANT_UTILS_H

#include "Common/MyString.h"
#include "PropVariant.h"

struct CUInt32PCharPair
{
  UInt32 Value;
  const char *Name;
};

void StringToProp(const AString &s, NWindows::NCOM::CPropVariant &prop);
void PairToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop);

AString FlagsToString(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags);
void FlagsToProp(const CUInt32PCharPair *pairs, unsigned num, UInt32 flags, NWindows::NCOM::CPropVariant &prop);

AString TypeToString(const char *table[], unsigned num, UInt32 value);
void TypeToProp(const char *table[], unsigned num, UInt32 value, NWindows::NCOM::CPropVariant &prop);

#define PAIR_TO_PROP(pairs, value, prop) PairToProp(pairs, sizeof(pairs) / sizeof(pairs[0]), value, prop)
#define FLAGS_TO_PROP(pairs, value, prop) FlagsToProp(pairs, sizeof(pairs) / sizeof(pairs[0]), value, prop)
#define TYPE_TO_PROP(table, value, prop) TypeToProp(table, sizeof(table) / sizeof(table[0]), value, prop)

#endif
