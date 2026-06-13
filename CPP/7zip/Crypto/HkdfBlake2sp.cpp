// HkdfBlake2sp.cpp
// Copyright (C) fzxx   Contributor: https://github.com/fzxx
// License: GNU LGPL v2.1+

#include "StdAfx.h"

#include "HkdfBlake2sp.h"

namespace NCrypto {
namespace NHkdfBlake2sp {

#define BLAKE2SP_BLOCK_SIZE 64

// HMAC-BLAKE2sp
static void HmacBlake2sp(const Byte *key, unsigned keySize,
    const Byte *message, unsigned messageSize,
    Byte *mac)
{
  static bool blake2spPrepared = false;
  if (!blake2spPrepared)
  {
    z7_Black2sp_Prepare();
    blake2spPrepared = true;
  }

  Byte ipad[BLAKE2SP_BLOCK_SIZE];
  Byte opad[BLAKE2SP_BLOCK_SIZE];

  memset(ipad, 0x36, BLAKE2SP_BLOCK_SIZE);
  memset(opad, 0x5c, BLAKE2SP_BLOCK_SIZE);

  for (unsigned i = 0; i < keySize && i < BLAKE2SP_BLOCK_SIZE; i++)
  {
    ipad[i] ^= key[i];
    opad[i] ^= key[i];
  }

  // Inner hash
  CAlignedBuffer1 bufInner(sizeof(CBlake2sp));
  CBlake2sp *blake2spInner = (CBlake2sp *)(void *)(Byte *)bufInner;
  Blake2sp_Init(blake2spInner);
  Blake2sp_SetFunction(blake2spInner, 0);
  Blake2sp_Update(blake2spInner, ipad, BLAKE2SP_BLOCK_SIZE);
  Blake2sp_Update(blake2spInner, message, messageSize);

  Byte innerHash[Z7_BLAKE2S_DIGEST_SIZE];
  Blake2sp_Final(blake2spInner, innerHash);

  // Outer hash
  CAlignedBuffer1 bufOuter(sizeof(CBlake2sp));
  CBlake2sp *blake2spOuter = (CBlake2sp *)(void *)(Byte *)bufOuter;
  Blake2sp_Init(blake2spOuter);
  Blake2sp_SetFunction(blake2spOuter, 0);
  Blake2sp_Update(blake2spOuter, opad, BLAKE2SP_BLOCK_SIZE);
  Blake2sp_Update(blake2spOuter, innerHash, Z7_BLAKE2S_DIGEST_SIZE);
  Blake2sp_Final(blake2spOuter, mac);

  Z7_memset_0_ARRAY(ipad);
  Z7_memset_0_ARRAY(opad);
  Z7_memset_0_ARRAY(innerHash);
}

// HKDF-Expand (RFC 5869)
void Derive(const Byte *prk, unsigned prkSize,
    const char *info, unsigned infoLen,
    Byte *output, unsigned outSize)
{
  const unsigned n = (outSize + Z7_BLAKE2S_DIGEST_SIZE - 1) / Z7_BLAKE2S_DIGEST_SIZE;

  Byte prevT[Z7_BLAKE2S_DIGEST_SIZE];
  unsigned prevTSize = 0;

  Byte *outPtr = output;
  unsigned remaining = outSize;

  for (unsigned i = 1; i <= n; i++)
  {
    Byte message[Z7_BLAKE2S_DIGEST_SIZE + 256 + 1];
    unsigned messageSize = 0;

    if (prevTSize > 0)
    {
      memcpy(message + messageSize, prevT, prevTSize);
      messageSize += prevTSize;
    }

    if (infoLen > 0)
    {
      memcpy(message + messageSize, info, infoLen);
      messageSize += infoLen;
    }

    message[messageSize] = (Byte)i;
    messageSize += 1;

    Byte ti[Z7_BLAKE2S_DIGEST_SIZE];
    HmacBlake2sp(prk, prkSize, message, messageSize, ti);

    const unsigned copySize = remaining < Z7_BLAKE2S_DIGEST_SIZE ? remaining : Z7_BLAKE2S_DIGEST_SIZE;
    memcpy(outPtr, ti, copySize);
    outPtr += copySize;
    remaining -= copySize;

    memcpy(prevT, ti, Z7_BLAKE2S_DIGEST_SIZE);
    prevTSize = Z7_BLAKE2S_DIGEST_SIZE;

    Z7_memset_0_ARRAY(ti);
    Z7_memset_0_ARRAY(message);
  }

  Z7_memset_0_ARRAY(prevT);
}

}}
