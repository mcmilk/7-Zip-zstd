// Crypto/ZipCrypto.h

#pragma once

#ifndef __CRYPTO_ZIP_CRYPTO_H
#define __CRYPTO_ZIP_CRYPTO_H

namespace NCrypto {
namespace NZip {

const kHeaderSize = 12;
class CCipher
{
  UINT32 Keys[3];
  void UpdateKeys(BYTE b);
  BYTE DecryptByteSpec();
public:
  void SetPassword(const BYTE *password, UINT32 passwordLength);
  BYTE DecryptByte(BYTE encryptedByte);
  BYTE EncryptByte(BYTE b);
  void DecryptHeader(BYTE *buffer);
  void EncryptHeader(BYTE *buffer);

};

}}

#endif
