// RandGen.h

#ifndef __CRYPTO_RAND_GEN_H
#define __CRYPTO_RAND_GEN_H

#include "Sha1.h"

class CRandomGenerator
{
  Byte _buff[NCrypto::NSha1::kDigestSize];
  bool _needInit;

  void Init();
public:
  CRandomGenerator(): _needInit(true) {};
  void Generate(Byte *data, unsigned size);
};

extern CRandomGenerator g_RandomGenerator;

#endif
