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

void CCipher::UpdateKeys(BYTE b)
{
  Keys[0] = CRC32(Keys[0], b);
  Keys[1] += Keys[0] & 0xff;
  Keys[1] = Keys[1] * 134775813L + 1;
  Keys[2] = CRC32(Keys[2], Keys[1] >> 24);
}

void CCipher::SetPassword(const BYTE *password, UINT32 passwordLength)
{
  Keys[0] = 305419896L;
  Keys[1] = 591751049L;
  Keys[2] = 878082192L;
  for (UINT32 i = 0; i < passwordLength; i++)
    UpdateKeys(password[i]);
}

BYTE CCipher::DecryptByteSpec()
{
  UINT32 temp = Keys[2] | 2;
  return (temp * (temp ^ 1)) >> 8;
}

BYTE CCipher::DecryptByte(BYTE encryptedByte)
{
  BYTE c = encryptedByte ^ DecryptByteSpec();
  UpdateKeys(c);
  return c;
}

BYTE CCipher::EncryptByte(BYTE b)
{
  BYTE c = b ^ DecryptByteSpec();
  UpdateKeys(b);
  return c;
}

void CCipher::DecryptHeader(BYTE *buffer)
{
  for (int i = 0; i < 12; i++)
    buffer[i] = DecryptByte(buffer[i]);
}

void CCipher::EncryptHeader(BYTE *buffer)
{
  for (int i = 0; i < 12; i++)
    buffer[i] = EncryptByte(buffer[i]);
}

}}
