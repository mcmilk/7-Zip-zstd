// Sha1.cpp
// This file is based on public domain 
// Steve Reid and Wei Dai's code from Crypto++

#include "StdAfx.h"

#include "Sha1.h"
#include "RotateDefs.h"

namespace NCrypto {
namespace NSha1 {

// define it for speed optimization
// #define _SHA1_UNROLL

static const unsigned int kNumW = 
  #ifdef _SHA1_UNROLL
  16;
  #else
  80;
  #endif
  

#define w0(i) (W[(i)] = data[(i)])

#ifdef _SHA1_UNROLL
#define w1(i) (W[(i)&15] = rotlFixed(W[((i)-3)&15] ^ W[((i)-8)&15] ^ W[((i)-14)&15] ^ W[((i)-16)&15], 1))
#else
#define w1(i) (W[(i)] = rotlFixed(W[(i)-3] ^ W[(i)-8] ^ W[(i)-14] ^ W[(i)-16], 1))
#endif

#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

#define RK1(a,b,c,d,e,i, f, w, k) e += f(b,c,d) + w(i) + k + rotlFixed(a,5); b = rotlFixed(b,30);

#define R0(a,b,c,d,e,i) RK1(a,b,c,d,e,i, f1, w0, 0x5A827999)
#define R1(a,b,c,d,e,i) RK1(a,b,c,d,e,i, f1, w1, 0x5A827999)
#define R2(a,b,c,d,e,i) RK1(a,b,c,d,e,i, f2, w1, 0x6ED9EBA1)
#define R3(a,b,c,d,e,i) RK1(a,b,c,d,e,i, f3, w1, 0x8F1BBCDC)
#define R4(a,b,c,d,e,i) RK1(a,b,c,d,e,i, f4, w1, 0xCA62C1D6)

#define RX_1_4(rx1, rx4, i) rx1(a,b,c,d,e,i); rx4(e,a,b,c,d,i+1); rx4(d,e,a,b,c,i+2); rx4(c,d,e,a,b,i+3); rx4(b,c,d,e,a,i+4);
#define RX_5(rx, i) RX_1_4(rx, rx, i);

void CContextBase::Init()
{
  _state[0] = 0x67452301;
  _state[1] = 0xEFCDAB89;
  _state[2] = 0x98BADCFE;
  _state[3] = 0x10325476;
  _state[4] = 0xC3D2E1F0;
  _count = 0;
}

void CContextBase::GetBlockDigest(UInt32 *data, UInt32 *destDigest, bool returnRes)
{
  UInt32 a, b, c, d, e;
  UInt32 W[kNumW];

  a = _state[0];
  b = _state[1];
  c = _state[2];
  d = _state[3];
  e = _state[4];
  #ifdef _SHA1_UNROLL
  RX_5(R0, 0); RX_5(R0, 5); RX_5(R0, 10);
  #else
  int i;
  for (i = 0; i < 15; i += 5) { RX_5(R0, i); }
  #endif

  RX_1_4(R0, R1, 15);


  #ifdef _SHA1_UNROLL
  RX_5(R2, 20); RX_5(R2, 25); RX_5(R2, 30); RX_5(R2, 35); 
  RX_5(R3, 40); RX_5(R3, 45); RX_5(R3, 50); RX_5(R3, 55); 
  RX_5(R4, 60); RX_5(R4, 65); RX_5(R4, 70); RX_5(R4, 75); 
  #else
  i = 20;
  for (; i < 40; i += 5) { RX_5(R2, i); }
  for (; i < 60; i += 5) { RX_5(R3, i); }
  for (; i < 80; i += 5) { RX_5(R4, i); }
  #endif

  destDigest[0] = _state[0] + a;
  destDigest[1] = _state[1] + b;
  destDigest[2] = _state[2] + c;
  destDigest[3] = _state[3] + d;
  destDigest[4] = _state[4] + e;

  if (returnRes)
    for (int i = 0 ; i < 16; i++)
      data[i] = W[kNumW - 16 + i];
  
  // Wipe variables
  // a = b = c = d = e = 0;
}  

void CContextBase::PrepareBlock(UInt32 *block, unsigned int size) const
{
  unsigned int curBufferPos = size & 0xF;
  block[curBufferPos++] = 0x80000000;
  while (curBufferPos != (16 - 2))
    block[curBufferPos++] = 0;
  const UInt64 lenInBits = (_count << 9) + ((UInt64)size << 5);
  block[curBufferPos++] = (UInt32)(lenInBits >> 32);
  block[curBufferPos++] = (UInt32)(lenInBits);
}

void CContext::Update(Byte *data, size_t size, bool rar350Mode)
{
  bool returnRes = false;
  unsigned int curBufferPos = _count2;
  while (size-- > 0)
  {
    int pos = (int)(curBufferPos & 3);
    if (pos == 0)
      _buffer[curBufferPos >> 2] = 0;
    _buffer[curBufferPos >> 2] |= ((UInt32)*data++) << (8 * (3 - pos));
    if (++curBufferPos == kBlockSize)
    {
      curBufferPos = 0;
      CContextBase::UpdateBlock(_buffer, returnRes);
      if (returnRes)
        for (int i = 0; i < kBlockSizeInWords; i++)
        {
          UInt32 d = _buffer[i];
          data[i * 4 + 0 - kBlockSize] = (Byte)(d);
          data[i * 4 + 1 - kBlockSize] = (Byte)(d >>  8);
          data[i * 4 + 2 - kBlockSize] = (Byte)(d >> 16);
          data[i * 4 + 3 - kBlockSize] = (Byte)(d >> 24);
        }
      returnRes = rar350Mode;
    }
  }
  _count2 = curBufferPos;
}

void CContext::Final(Byte *digest)
{
  const UInt64 lenInBits = (_count << 9) + ((UInt64)_count2 << 3);
  unsigned int curBufferPos = _count2;
  int pos = (int)(curBufferPos & 3);
  curBufferPos >>= 2;
  if (pos == 0)
    _buffer[curBufferPos] = 0;
  _buffer[curBufferPos++] |= ((UInt32)0x80) << (8 * (3 - pos));

  while (curBufferPos != (16 - 2))
  {
    curBufferPos &= 0xF;
    if (curBufferPos == 0)
      UpdateBlock();
    _buffer[curBufferPos++] = 0;
  }
  _buffer[curBufferPos++] = (UInt32)(lenInBits >> 32);
  _buffer[curBufferPos++] = (UInt32)(lenInBits);
  UpdateBlock();

  int i;
  for (i = 0; i < kDigestSizeInWords; i++) 
  {
    UInt32 state = _state[i] & 0xFFFFFFFF;
    *digest++ = (Byte)(state >> 24);
    *digest++ = (Byte)(state >> 16);
    *digest++ = (Byte)(state >> 8);
    *digest++ = (Byte)(state);
  }
  Init();
}

///////////////////////////
// Words version

void CContext32::Update(const UInt32 *data, size_t size)
{
  while (size-- > 0)
  {
    _buffer[_count2++] = *data++;
    if (_count2 == kBlockSizeInWords)
    {
      _count2 = 0;
      UpdateBlock();
    }
  }
}

void CContext32::Final(UInt32 *digest)
{
  const UInt64 lenInBits = (_count << 9) + ((UInt64)_count2 << 5);
  unsigned int curBufferPos = _count2;
  _buffer[curBufferPos++] = 0x80000000;
  while (curBufferPos != (16 - 2))
  {
    curBufferPos &= 0xF;
    if (curBufferPos == 0)
      UpdateBlock();
    _buffer[curBufferPos++] = 0;
  }
  _buffer[curBufferPos++] = (UInt32)(lenInBits >> 32);
  _buffer[curBufferPos++] = (UInt32)(lenInBits);
  GetBlockDigest(_buffer, digest);
  Init();
}

}}
