// MethodInfo.cpp

#include "StdAfx.h"

#include "MethodInfo.h"

namespace NArchive {
namespace N7z {

inline char GetHex(BYTE aValue)
{
  return (aValue < 10) ? ('0' + aValue) : ('A' + (aValue - 10));
}

bool HexCharToInt(char aValue, BYTE &aResult)
{
  if (aValue >= '0' && aValue <= '9')
    aResult = aValue - '0';
  else if (aValue >= 'a' && aValue <= 'f')
    aResult = 10 + aValue - 'a';
  else if (aValue >= 'A' && aValue <= 'F')
    aResult = 10 + aValue - 'A';
  else
    return false;
  return true;
}

bool TwoHexCharsToInt(char aValueHigh, char aValueLow, BYTE &aResult)
{
  BYTE aResultHigh, aResultLow;
  if (!HexCharToInt(aValueHigh, aResultHigh))
    return false;
  if (!HexCharToInt(aValueLow, aResultLow))
    return false;
  aResult = (aResultHigh << 4) + aResultLow;
  return true;
}

AString CMethodID::ConvertToString() const
{
  AString aResult;
  for (int i = 0; i < IDSize; i++)
  {
    BYTE aByte = ID[i];
    aResult += GetHex(aByte >> 4);
    aResult += GetHex(aByte & 0xF);
  }
  return aResult;
}

bool CMethodID::ConvertFromString(const AString &aString)
{
  int aLength = aString.Length();
  if ((aLength & 1) != 0)
    return false;
  IDSize = aLength / 2;
  if (IDSize > kMethodIDSize)
    return false;
  UINT32 i;
  for(i = 0; i < IDSize; i++)
    if (!TwoHexCharsToInt(aString[i * 2], aString[i * 2 + 1], ID[i]))
      return false;
  for(i = IDSize; i < kMethodIDSize; i++)
    ID[i] = 0;
  return true;
}

}}