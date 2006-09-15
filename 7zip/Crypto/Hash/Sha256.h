// Crypto/Sha256.h

#ifndef __CRYPTO_SHA256_H
#define __CRYPTO_SHA256_H

#include "Common/Types.h"

namespace NCrypto {
namespace NSha256 {

class CContext
{
  static const UInt32 K[64];

  UInt32 _state[8];
  UInt64 _count;
  Byte _buffer[64];
  static void Transform(UInt32 *digest, const UInt32 *data);
  void WriteByteBlock();
public:
  enum {DIGESTSIZE = 32};
  CContext() { Init(); } ;
  void Init();
  void Update(const Byte *data, size_t size);
  void Final(Byte *digest);
};

}}

#endif
