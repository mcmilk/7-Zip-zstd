// Crypto/Zip/Crypto.h

#pragma once

#ifndef __CRYPTO_ZIP_CRYPTO_H
#define __CRYPTO_ZIP_CRYPTO_H

namespace NCrypto {
namespace NZip {

const kHeaderSize = 12;
class CData
{
  UINT32 Keys[3];
  void UpdateKeys(BYTE c);
  BYTE DecryptByteSpec();
public:
  void SetPassword(const BYTE *aPassword, UINT32 aPasswordLength);
  void DecryptHeader(BYTE *Buf);
  BYTE DecryptByte(BYTE anEncryptedByte);
};

}}

#endif
