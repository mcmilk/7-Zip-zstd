// AES_CBC.h

#ifndef __AES_CBC_H
#define __AES_CBC_H

#include "aescpp.h"

class CAES_CBCEncoder: public AESclass
{
  BYTE _prevBlock[16];
public:
  void Init(const BYTE *iv)
  {
    for (int i = 0; i < 16; i++)
      _prevBlock[i] = iv[i];
  }
  void ProcessData(BYTE *outBlock, const BYTE *inBlock)
  {
    int i;
    for (i = 0; i < 16; i++)
      _prevBlock[i] ^= inBlock[i];
    enc_blk(_prevBlock, outBlock);
    for (i = 0; i < 16; i++)
      _prevBlock[i] = outBlock[i];
  }
};

class CAES_CBCCBCDecoder: public AESclass
{
  BYTE _prevBlock[16];
public:
  void Init(const BYTE *iv)
  {
    for (int i = 0; i < 16; i++)
      _prevBlock[i] = iv[i];
  }
  void ProcessData(BYTE *outBlock, const BYTE *inBlock)
  {
    dec_blk(inBlock, outBlock);
    int i;
    for (i = 0; i < 16; i++)
      outBlock[i] ^= _prevBlock[i];
    for (i = 0; i < 16; i++)
      _prevBlock[i] = inBlock[i];
  }
};

#endif
