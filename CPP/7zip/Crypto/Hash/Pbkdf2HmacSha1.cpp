// Pbkdf2HmacSha1.cpp

#include "StdAfx.h"

#include "HmacSha1.h"

namespace NCrypto {
namespace NSha1 {

void Pbkdf2Hmac(const Byte *pwd, size_t pwdSize, const Byte *salt, size_t saltSize, 
    UInt32 numIterations, Byte *key, size_t keySize)
{
  CHmac baseCtx;
  baseCtx.SetKey(pwd, pwdSize);
  for (UInt32 i = 1; keySize > 0; i++)
  {
    CHmac ctx = baseCtx;
    ctx.Update(salt, saltSize);
    Byte u[kDigestSize] = { (Byte)(i >> 24), (Byte)(i >> 16), (Byte)(i >> 8), (Byte)(i) };
    const unsigned int curSize = (keySize < kDigestSize) ? (unsigned int)keySize : kDigestSize;
    ctx.Update(u, 4);
    ctx.Final(u, kDigestSize);

    unsigned int s;
    for (s = 0; s < curSize; s++)
      key[s] = u[s];
    
    for (UInt32 j = numIterations; j > 1; j--)
    {
      ctx = baseCtx;
      ctx.Update(u, kDigestSize);
      ctx.Final(u, kDigestSize);
      for (s = 0; s < curSize; s++)
        key[s] ^= u[s];
    }

    key += curSize;
    keySize -= curSize;
  }
}

void Pbkdf2Hmac32(const Byte *pwd, size_t pwdSize, const UInt32 *salt, size_t saltSize, 
    UInt32 numIterations, UInt32 *key, size_t keySize)
{
  CHmac32 baseCtx;
  baseCtx.SetKey(pwd, pwdSize);
  for (UInt32 i = 1; keySize > 0; i++)
  {
    CHmac32 ctx = baseCtx;
    ctx.Update(salt, saltSize);
    UInt32 u[kDigestSizeInWords] = { i };
    const unsigned int curSize = (keySize < kDigestSizeInWords) ? (unsigned int)keySize : kDigestSizeInWords;
    ctx.Update(u, 1);
    ctx.Final(u, kDigestSizeInWords);

    // Speed-optimized code start
    ctx = baseCtx;
    ctx.GetLoopXorDigest(u, numIterations - 1);
    // Speed-optimized code end
    
    unsigned int s;
    for (s = 0; s < curSize; s++)
      key[s] = u[s];
    
    /*
    // Default code start
    for (UInt32 j = numIterations; j > 1; j--)
    {
      ctx = baseCtx;
      ctx.Update(u, kDigestSizeInWords);
      ctx.Final(u, kDigestSizeInWords);
      for (s = 0; s < curSize; s++)
        key[s] ^= u[s];
    }
    // Default code end
    */

    key += curSize;
    keySize -= curSize;
  }
}

}}
