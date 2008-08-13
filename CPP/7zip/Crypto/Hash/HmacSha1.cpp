// HmacSha1.cpp

#include "StdAfx.h"

#include "HmacSha1.h"

namespace NCrypto {
namespace NSha1 {

void CHmac::SetKey(const Byte *key, size_t keySize)
{
  Byte keyTemp[kBlockSize];
  size_t i;
  for (i = 0; i < kBlockSize; i++)
    keyTemp[i] = 0;
  if(keySize > kBlockSize)
  {
    _sha.Init();
    _sha.Update(key, keySize);
    _sha.Final(keyTemp);
    keySize = kDigestSize;
  }
  else
    for (i = 0; i < keySize; i++)
      keyTemp[i] = key[i];
  for (i = 0; i < kBlockSize; i++)
    keyTemp[i] ^= 0x36;
  _sha.Init();
  _sha.Update(keyTemp, kBlockSize);
  for (i = 0; i < kBlockSize; i++)
    keyTemp[i] ^= 0x36 ^ 0x5C;
  _sha2.Init();
  _sha2.Update(keyTemp, kBlockSize);
}

void CHmac::Final(Byte *mac, size_t macSize)
{
  Byte digest[kDigestSize];
  _sha.Final(digest);
  _sha2.Update(digest, kDigestSize);
  _sha2.Final(digest);
  for(size_t i = 0; i < macSize; i++)
    mac[i] = digest[i];
}


void CHmac32::SetKey(const Byte *key, size_t keySize)
{
  UInt32 keyTemp[kBlockSizeInWords];
  size_t i;
  for (i = 0; i < kBlockSizeInWords; i++)
    keyTemp[i] = 0;
  if(keySize > kBlockSize)
  {
    CContext sha;
    sha.Init();
    sha.Update(key, keySize);
    Byte digest[kDigestSize];
    sha.Final(digest);
    
    for (int i = 0 ; i < kDigestSizeInWords; i++)
      keyTemp[i] =
          ((UInt32)(digest[i * 4 + 0]) << 24) |
          ((UInt32)(digest[i * 4 + 1]) << 16) |
          ((UInt32)(digest[i * 4 + 2]) <<  8) |
          ((UInt32)(digest[i * 4 + 3]));
    keySize = kDigestSizeInWords;
  }
  else
    for (size_t i = 0; i < keySize; i++)
      keyTemp[i / 4] |= (key[i] << (24 - 8 * (i & 3)));
  for (i = 0; i < kBlockSizeInWords; i++)
    keyTemp[i] ^= 0x36363636;
  _sha.Init();
  _sha.Update(keyTemp, kBlockSizeInWords);
  for (i = 0; i < kBlockSizeInWords; i++)
    keyTemp[i] ^= 0x36363636 ^ 0x5C5C5C5C;
  _sha2.Init();
  _sha2.Update(keyTemp, kBlockSizeInWords);
}

void CHmac32::Final(UInt32 *mac, size_t macSize)
{
  UInt32 digest[kDigestSizeInWords];
  _sha.Final(digest);
  _sha2.Update(digest, kDigestSizeInWords);
  _sha2.Final(digest);
  for(size_t i = 0; i < macSize; i++)
    mac[i] = digest[i];
}

void CHmac32::GetLoopXorDigest(UInt32 *mac, UInt32 numIteration)
{
  UInt32 block[kBlockSizeInWords];
  UInt32 block2[kBlockSizeInWords];
  _sha.PrepareBlock(block, kDigestSizeInWords);
  _sha2.PrepareBlock(block2, kDigestSizeInWords);
  for(unsigned int s = 0; s < kDigestSizeInWords; s++)
    block[s] = mac[s];
  for(UInt32 i = 0; i < numIteration; i++)
  {
    _sha.GetBlockDigest(block, block2);
    _sha2.GetBlockDigest(block2, block);
    for (unsigned int s = 0; s < kDigestSizeInWords; s++)
      mac[s] ^= block[s];
  }
}

}}
