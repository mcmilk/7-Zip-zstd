// HmacSha512.h
// Copyright (C) fzxx   Contributor: https://github.com/fzxx
// License: GNU LGPL v2.1+

#ifndef ZIP7_INC_CRYPTO_HMAC_SHA512_H
#define ZIP7_INC_CRYPTO_HMAC_SHA512_H

#include "../../../C/Sha512.h"

namespace NCrypto {
namespace NSha512 {

const unsigned kBlockSize = SHA512_BLOCK_SIZE;
const unsigned kDigestSize = SHA512_DIGEST_SIZE;
const unsigned kNumDigestWords = SHA512_NUM_DIGEST_WORDS;

class CHmac
{
  CSha512 _sha;
  CSha512 _sha2;

  // wipes key material; volatile access keeps the stores alive
  static void WipeObj(CSha512 &s)
  {
    volatile Byte *p = (volatile Byte *)(void *)&s;
    for (size_t i = 0; i < sizeof(s); i++)
      p[i] = 0;
  }
public:
  CHmac() {}
#ifdef Z7_CPP_IS_SUPPORTED_default
  CHmac(const CHmac &) = default;
  CHmac &operator=(const CHmac &) = default;
#endif
  ~CHmac()
  {
    WipeObj(_sha);
    WipeObj(_sha2);
  }
  void SetKey(const Byte *key, size_t keySize);
  void Update(const Byte *data, size_t dataSize) { Sha512_Update(&_sha, data, dataSize); }
  void Final(Byte *mac);
};

}}

#endif
