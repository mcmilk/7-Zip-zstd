// MethodInfo.cpp

#include "StdAfx.h"

#include "MethodInfo.h"

namespace NArchive {
namespace N7z {

inline char GetHex(BYTE value)
{
  return (value < 10) ? ('0' + value) : ('A' + (value - 10));
}

bool HexCharToInt(char value, BYTE &result)
{
  if (value >= '0' && value <= '9')
    result = value - '0';
  else if (value >= 'a' && value <= 'f')
    result = 10 + value - 'a';
  else if (value >= 'A' && value <= 'F')
    result = 10 + value - 'A';
  else
    return false;
  return true;
}

bool TwoHexCharsToInt(char valueHigh, char valueLow, BYTE &result)
{
  BYTE resultHigh, resultLow;
  if (!HexCharToInt(valueHigh, resultHigh))
    return false;
  if (!HexCharToInt(valueLow, resultLow))
    return false;
  result = (resultHigh << 4) + resultLow;
  return true;
}

AString CMethodID::ConvertToString() const
{
  AString result;
  for (int i = 0; i < IDSize; i++)
  {
    BYTE b = ID[i];
    result += GetHex(b >> 4);
    result += GetHex(b & 0xF);
  }
  return result;
}

bool CMethodID::ConvertFromString(const AString &srcString)
{
  int length = srcString.Length();
  if ((length & 1) != 0)
    return false;
  IDSize = length / 2;
  if (IDSize > kMethodIDSize)
    return false;
  UINT32 i;
  for(i = 0; i < IDSize; i++)
    if (!TwoHexCharsToInt(srcString[i * 2], srcString[i * 2 + 1], ID[i]))
      return false;
  for(i = IDSize; i < kMethodIDSize; i++)
    ID[i] = 0;
  return true;
}

}}