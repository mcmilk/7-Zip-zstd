// Crypto/SHA256.cpp
// This code is based on code from Wei Dai's Crypto++ library.

#include "StdAfx.h"

#include "SHA256.h"
#include "Windows/Defs.h"

const int kBufferSize = 1 << 17;

namespace NCrypto {
namespace NSHA256 {


template <class T> static inline T rotrFixed(T x, unsigned int y)
{
  // assert(y < sizeof(T)*8);
  return (x>>y) | (x<<(sizeof(T)*8-y));
}

#define blk0(i) (W[i] = data[i])


void SHA256::Init()
{
  m_digest[0] = 0x6a09e667;
  m_digest[1] = 0xbb67ae85;
  m_digest[2] = 0x3c6ef372;
  m_digest[3] = 0xa54ff53a;
  m_digest[4] = 0x510e527f;
  m_digest[5] = 0x9b05688c;
  m_digest[6] = 0x1f83d9ab;
  m_digest[7] = 0x5be0cd19;
  
  m_count = 0;
}

#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+K[i+j]+(j?blk2(i):blk0(i));\
  d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))

// for SHA256
#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))

void SHA256::Transform(UInt32 *state, const UInt32 *data)
{
  UInt32 W[16];
  UInt32 T[8];
  /* Copy context->state[] to working vars */
  // memcpy(T, state, sizeof(T));
  for (int s = 0; s < 8; s++)
    T[s] = state[s];


  /* 64 operations, partially loop unrolled */
  for (unsigned int j = 0; j < 64; j += 16)
  {
    for (unsigned int i = 0; i < 16; i++)
    {
      R(i);
    }
    /*
    R( 0); R( 1); R( 2); R( 3);
    R( 4); R( 5); R( 6); R( 7);
    R( 8); R( 9); R(10); R(11);
    R(12); R(13); R(14); R(15);
    */
  }
  /* Add the working vars back into context.state[] */
  /*
  state[0] += a(0);
  state[1] += b(0);
  state[2] += c(0);
  state[3] += d(0);
  state[4] += e(0);
  state[5] += f(0);
  state[6] += g(0);
  state[7] += h(0);
  */
  for (int i = 0; i < 8; i++)
    state[i] += T[i];
  
  /* Wipe variables */
  // memset(W, 0, sizeof(W));
  // memset(T, 0, sizeof(T));
}

const UInt32 SHA256::K[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#undef S0
#undef S1
#undef s0
#undef s1

void SHA256::WriteByteBlock()
{
  UInt32 data32[16];
  for (int i = 0; i < 16; i++)
  {
    data32[i] = (UInt32(_buffer[i * 4]) << 24) +
      (UInt32(_buffer[i * 4 + 1]) << 16) +
      (UInt32(_buffer[i * 4 + 2]) << 8) +
      UInt32(_buffer[i * 4 + 3]);
  }
  Transform(m_digest, data32);
}

void SHA256::Update(const Byte *data, UInt32 size)
{
  UInt32 curBufferPos = UInt32(m_count) & 0x3F;
  while (size > 0)
  {
    while(curBufferPos < 64 && size > 0)
    {
      _buffer[curBufferPos++] = *data++;
      m_count++;
      size--;
    }
    if (curBufferPos == 64)
    {
      curBufferPos = 0;
      WriteByteBlock();
    }
  }
}

void SHA256::Final(Byte *digest)
{
  UInt64 lenInBits = (m_count << 3);
  UInt32 curBufferPos = UInt32(m_count) & 0x3F;
  _buffer[curBufferPos++] = 0x80;
  while (curBufferPos != (64 - 8))
  {
    curBufferPos &= 0x3F;
    if (curBufferPos == 0)
      WriteByteBlock();
    _buffer[curBufferPos++] = 0;
  }
  for (int i = 0; i < 8; i++)
  {
    _buffer[curBufferPos++] = Byte(lenInBits >> 56);
    lenInBits <<= 8;
  }
  WriteByteBlock();

  for (int j = 0; j < 8; j++)
  {
    *digest++ = m_digest[j] >> 24;
    *digest++ = m_digest[j] >> 16;
    *digest++ = m_digest[j] >> 8;
    *digest++ = m_digest[j];
  }
  Init();
}

}}
