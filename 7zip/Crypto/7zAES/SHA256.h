// Crypto/SHA256.h

#ifndef __CRYPTO_SHA256_H
#define __CRYPTO_SHA256_H

#include "Common/Types.h"

namespace NCrypto {
namespace NSHA256 {

class SHA256
{
  static const UInt32 K[64];

  UInt32 m_digest[8];
  UInt64 m_count;
  Byte _buffer[64];
  static void Transform(UInt32 *digest, const UInt32 *data);
  void WriteByteBlock();
public:
  enum {DIGESTSIZE = 32};
  SHA256() { Init(); } ;
  void Init();
  void Update(const Byte *data, UInt32 size);
  void Final(Byte *digest);
};

}}

#endif