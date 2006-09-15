// RandGen.h

#ifndef __RANDGEN_H
#define __RANDGEN_H

#include "Sha1.h"

class CRandomGenerator
{
  Byte _buff[NCrypto::NSha1::kDigestSize];
  bool _needInit;

  void Init();
public:
  CRandomGenerator(): _needInit(true) {};
  void Generate(Byte *data, unsigned int size);
};

extern CRandomGenerator g_RandomGenerator;

#endif
