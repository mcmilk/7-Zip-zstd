// sha1.cpp
// This file is based on public domain 
// Steve Reid and Wei Dai's code from Crypto++

#include "StdAfx.h"

#include "sha1.h"

static inline UInt32 rotlFixed(UInt32 x, int n)
{
	return (x << n) | (x >> (32 - n));
}

#define blk0(i) (W[i] = data[i])
#define blk1(i) (W[i&15] = rotlFixed(W[(i+13)&15]^W[(i+8)&15]^W[(i+2)&15]^W[i&15],1))

#define f1(x,y,z) (z^(x&(y^z)))
#define f2(x,y,z) (x^y^z)
#define f3(x,y,z) ((x&y)|(z&(x|y)))
#define f4(x,y,z) (x^y^z)

/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
#define R0(v,w,x,y,z,i) z+=f1(w,x,y)+blk0(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R1(v,w,x,y,z,i) z+=f1(w,x,y)+blk1(i)+0x5A827999+rotlFixed(v,5);w=rotlFixed(w,30);
#define R2(v,w,x,y,z,i) z+=f2(w,x,y)+blk1(i)+0x6ED9EBA1+rotlFixed(v,5);w=rotlFixed(w,30);
#define R3(v,w,x,y,z,i) z+=f3(w,x,y)+blk1(i)+0x8F1BBCDC+rotlFixed(v,5);w=rotlFixed(w,30);
#define R4(v,w,x,y,z,i) z+=f4(w,x,y)+blk1(i)+0xCA62C1D6+rotlFixed(v,5);w=rotlFixed(w,30);


/* Hash a single 512-bit block. This is the core of the algorithm. */

void CSHA1::Transform(UInt32 data[16], bool returnRes)
{
  UInt32 a, b, c, d, e;
	UInt32 W[16];

  /* Copy context->m_State[] to working vars */
  a = m_State[0];
  b = m_State[1];
  c = m_State[2];
  d = m_State[3];
  e = m_State[4];
  /* 4 rounds of 20 operations each. Loop unrolled. */
  R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
  R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
  R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
  R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
  R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
  R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
  R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
  R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
  R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
  R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
  R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
  R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
  R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
  R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
  R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
  R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
  R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
  R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
  R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
  R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
  /* Add the working vars back into context.m_State[] */
  m_State[0] += a;
  m_State[1] += b;
  m_State[2] += c;
  m_State[3] += d;
  m_State[4] += e;
  if (returnRes)
    for (int i = 0 ; i < 16; i++)
      data[i] = W[i];
  
  /* Wipe variables */
  a = b = c = d = e = 0;
}  


void CSHA1::Init()
{
  m_State[0] = 0x67452301;
  m_State[1] = 0xEFCDAB89;
  m_State[2] = 0x98BADCFE;
  m_State[3] = 0x10325476;
  m_State[4] = 0xC3D2E1F0;
  m_Count = 0;
}


void CSHA1::WriteByteBlock(bool returnRes)
{
  UInt32 data32[16];
  int i;
  for (i = 0; i < 16; i++)
  {
    data32[i] = 
      (UInt32(_buffer[i * 4 + 0]) << 24) +
      (UInt32(_buffer[i * 4 + 1]) << 16) +
      (UInt32(_buffer[i * 4 + 2]) <<  8) +
       UInt32(_buffer[i * 4 + 3]);
  }
  Transform(data32, returnRes);
  if (returnRes)
    for (i = 0; i < 16; i++)
    {
      UInt32 d = data32[i];
      _buffer[i * 4 + 0] = (Byte)(d >>  0);
      _buffer[i * 4 + 1] = (Byte)(d >>  8);
      _buffer[i * 4 + 2] = (Byte)(d >> 16);
      _buffer[i * 4 + 3] = (Byte)(d >> 24);
    }
}

void CSHA1::Update(Byte *data, size_t size, bool rar350Mode)
{
  bool returnRes = false;
  UInt32 curBufferPos = UInt32(m_Count) & 0x3F;
  while (size > 0)
  {
    while(curBufferPos < 64 && size > 0)
    {
      _buffer[curBufferPos++] = *data++;
      m_Count++;
      size--;
    }
    if (curBufferPos == 64)
    {
      curBufferPos = 0;
      WriteByteBlock(returnRes);
      if (returnRes)
        for (int i = 0; i < 64; i++)
          data[i - 64] = _buffer[i];
      returnRes = rar350Mode;
    }
  }
}

void CSHA1::Final(Byte *digest)
{
  UInt64 lenInBits = (m_Count << 3);
  UInt32 curBufferPos = UInt32(m_Count) & 0x3F;
  _buffer[curBufferPos++] = 0x80;
  while (curBufferPos != (64 - 8))
  {
    curBufferPos &= 0x3F;
    if (curBufferPos == 0)
      WriteByteBlock();
    _buffer[curBufferPos++] = 0;
  }
  int i;
  for (i = 0; i < 8; i++)
  {
    _buffer[curBufferPos++] = Byte(lenInBits >> 56);
    lenInBits <<= 8;
  }
  WriteByteBlock();

  for (i = 0; i < 5; i++) 
  {
    UInt32 state = m_State[i] & 0xffffffff;
    *digest++ = state >> 24;
    *digest++ = state >> 16;
    *digest++ = state >> 8;
    *digest++ = state;
  }
  Init();
}


