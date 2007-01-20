// Crypto/ZipCrypto.h

#ifndef __CRYPTO_ZIP_CRYPTO_H
#define __CRYPTO_ZIP_CRYPTO_H

namespace NCrypto {
namespace NZip {

const int kHeaderSize = 12;
class CCipher
{
  UInt32 Keys[3];
  void UpdateKeys(Byte b);
  Byte DecryptByteSpec();
public:
  void SetPassword(const Byte *password, UInt32 passwordLength);
  Byte DecryptByte(Byte encryptedByte);
  Byte EncryptByte(Byte b);
  void DecryptHeader(Byte *buffer);
  void EncryptHeader(Byte *buffer);

};

}}

#endif
