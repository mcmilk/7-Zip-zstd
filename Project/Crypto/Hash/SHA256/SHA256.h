// Crypto/SHA/SHA256.h

#ifndef __CRYPTO_SHA256_H
#define __CRYPTO_SHA256_H

#include "Common/Types.h"

namespace NCrypto {
namespace NSHA256 {

class SHA256
{
  static const UINT32 K[64];

  UINT32 m_digest[8];
  UINT64 m_count;
  BYTE _buffer[64];
  static void Transform(UINT32 *digest, const UINT32 *data);
  void WriteByteBlock();
public:
  enum {DIGESTSIZE = 32};
  SHA256() { Init(); } ;
  void Init();
  void Update(const BYTE *data, UINT32 size);
  void Final(BYTE *digest);
};

}}

#endif