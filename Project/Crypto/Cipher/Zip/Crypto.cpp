// Crypto/Zip/Crypto.cpp

#include "StdAfx.h"

#include "Crypto.h"
#include "Common/Crc.h"

namespace NCrypto {
namespace NZip {

inline UINT32 CRC32(UINT32 c, BYTE  b)
{
  return CCRC::Table[(c ^ b) & 0xFF] ^ (c >> 8);
}

void CData::UpdateKeys(BYTE c)
{
  Keys[0] = CRC32(Keys[0], c);
  Keys[1] += Keys[0] & 0xff;
  Keys[1] = Keys[1] * 134775813L + 1;
  Keys[2] = CRC32(Keys[2], Keys[1] >> 24);
}

void CData::SetPassword(const BYTE *aPassword, UINT32 aPasswordLength)
{
  Keys[0] = 305419896L;
  Keys[1] = 591751049L;
  Keys[2] = 878082192L;
  for (UINT32 i = 0; i < aPasswordLength; i++)
    UpdateKeys(aPassword[i]);
}

BYTE CData::DecryptByteSpec()
{
  UINT32 aTemp = Keys[2] | 2;
  return (aTemp * (aTemp ^ 1)) >> 8;
}

BYTE CData::DecryptByte(BYTE anEncryptedByte)
{
  BYTE c = anEncryptedByte ^ DecryptByteSpec();
  UpdateKeys(c);
  return c;
}

void CData::DecryptHeader(BYTE *aBuffer)
{
  for (int i = 0; i < 12; i++)
    aBuffer[i] = DecryptByte(aBuffer[i]);
}

}}
