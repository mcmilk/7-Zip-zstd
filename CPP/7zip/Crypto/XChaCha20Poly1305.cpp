// XChaCha20Poly1305.cpp
// Copyright (C) fzxx   Contributor: https://github.com/fzxx
// License: GNU LGPL v2.1+

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"

#ifndef Z7_ST
#include "../../Windows/Synchronization.h"
#endif

#include "../Common/StreamUtils.h"

#include "XChaCha20Poly1305.h"

#ifndef Z7_EXTRACT_ONLY
#include "RandGen.h"
#endif

#if defined(MY_CPU_AMD64)
  #ifdef _MSC_VER
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif
  #define Z7_POLY1305_128BIT
#endif

namespace NCrypto {
namespace NXChaCha20Poly1305 {

void CBaseCoder::ComputePolyKey()
{
  Byte polyBlock[64];
  NXChaCha20::XChaCha20Block_Core(polyBlock, _derivedKey, _nonce + 16, 0);
  memcpy(_polyKey, polyBlock, kPolyKeySize);
  Z7_memset_0_ARRAY(polyBlock);
}

CPoly1305::CPoly1305()
{
  Reset();
}

void CPoly1305::Reset()
{
  memset(_r, 0, sizeof(_r));
  memset(_s, 0, sizeof(_s));
  memset(_h, 0, sizeof(_h));
  memset(_block, 0, sizeof(_block));
  _blockPos = 0;
  _totalLen = 0;
  memset(_aadBlock, 0, sizeof(_aadBlock));
  _aadBlockPos = 0;
  _aadLen = 0;
  _finalized = false;
}

void CPoly1305::SetKey(const Byte *key)
{
  Reset();
  memcpy(_r, key, 16);
  _r[3] &= 15;
  _r[7] &= 15;
  _r[11] &= 15;
  _r[15] &= 15;
  _r[4] &= 252;
  _r[8] &= 252;
  _r[12] &= 252;

  memcpy(_s, key + 16, 16);
}

#ifdef Z7_POLY1305_128BIT

#if defined(MY_CPU_AMD64) && !defined(_MSC_VER)
/* GCC/Clang fallback for _umul128 */
static inline UInt64 Z7_umul128(UInt64 a, UInt64 b, UInt64 *hi)
{
  unsigned __int128 p = (unsigned __int128)a * b;
  *hi = (UInt64)(p >> 64);
  return (UInt64)p;
}
#define _umul128 Z7_umul128
/* GCC/Clang: _addcarry_u64 uses unsigned long long, cast UInt64* to match */
#define Z7_ADDCARRY_U64(c, x, y, out) \
  _addcarry_u64((c), (unsigned long long)(x), (unsigned long long)(y), (unsigned long long *)(out))
#elif defined(_MSC_VER)
#define Z7_ADDCARRY_U64(c, x, y, out) _addcarry_u64((c), (x), (y), (out))
#endif

static void Poly1305_ProcessBlock_128(Byte h[16], const Byte r[16], const Byte block[16], bool hasHighBit)
{
  UInt64 d0 = GetUi32(h);
  UInt64 d1 = GetUi32(h + 4);
  UInt64 d2 = GetUi32(h + 8);
  UInt64 d3 = GetUi32(h + 12) & 0x3FFFFFF;

  UInt64 m0 = d0 & 0x3FFFFFF;
  UInt64 m1 = (d0 >> 26) | ((d1 & 0xFFFFF) << 6);
  UInt64 m2 = (d1 >> 20) | ((d2 & 0x3FFF) << 12);
  UInt64 m3 = (d2 >> 14) | ((d3 & 0xFF) << 18);
  UInt64 m4 = (d3 >> 8) & 0x3FFFF;

  UInt64 msg_lo = GetUi64(block);
  UInt64 msg_hi = GetUi64(block + 8);
  UInt64 r0 = GetUi64(r);
  UInt64 r1 = GetUi64(r + 8);

#if defined(_M_AMD64) || defined(MY_CPU_AMD64)
  {
    UInt64 hv0 = m0 | (m1 << 26) | ((m2 & 0xFFF) << 52);
    UInt64 hv1 = (m2 >> 12) | (m3 << 14) | (m4 << 40);
    UInt64 hv2 = 0;

    unsigned char c;
    c = Z7_ADDCARRY_U64(0, hv0, msg_lo, &hv0);
    c = Z7_ADDCARRY_U64(c, hv1, msg_hi, &hv1);
    hv2 += c + (hasHighBit ? 1 : 0);

    UInt64 d0_hi, d0_lo = _umul128(hv0, r0, &d0_hi);
    UInt64 d1a_hi, d1a_lo = _umul128(hv0, r1, &d1a_hi);
    UInt64 d1b_hi, d1b_lo = _umul128(hv1, r0, &d1b_hi);
    UInt64 d2a_hi, d2a_lo = _umul128(hv1, r1, &d2a_hi);
    UInt64 d2b_hi, d2b_lo = _umul128(hv2, r0, &d2b_hi);
    UInt64 d3_hi, d3_lo = _umul128(hv2, r1, &d3_hi);

    UInt64 a0 = d0_lo, a1 = d0_hi, a2 = 0, a3 = 0;
    c = Z7_ADDCARRY_U64(0, a1, d1a_lo, &a1);
    c = Z7_ADDCARRY_U64(c, a2, d1a_hi, &a2);
    c = Z7_ADDCARRY_U64(c, a3, 0, &a3);
    c = Z7_ADDCARRY_U64(0, a1, d1b_lo, &a1);
    c = Z7_ADDCARRY_U64(c, a2, d1b_hi, &a2);
    c = Z7_ADDCARRY_U64(c, a3, 0, &a3);
    c = Z7_ADDCARRY_U64(0, a2, d2a_lo, &a2);
    c = Z7_ADDCARRY_U64(c, a3, d2a_hi, &a3);
    c = Z7_ADDCARRY_U64(0, a2, d2b_lo, &a2);
    c = Z7_ADDCARRY_U64(c, a3, d2b_hi, &a3);
    UInt64 a4 = c;
    c = Z7_ADDCARRY_U64(0, a3, d3_lo, &a3);
    c = Z7_ADDCARRY_U64(c, a4, d3_hi, &a4);

    UInt64 hi[3];
    hi[0] = (a2 >> 2) | (a3 << 62);
    hi[1] = (a3 >> 2) | (a4 << 62);
    hi[2] = a4 >> 2;

    UInt64 h5_0_hi, h5_0 = _umul128(hi[0], 5, &h5_0_hi);
    UInt64 h5_1_hi, h5_1 = _umul128(hi[1], 5, &h5_1_hi);
    UInt64 h5_2 = hi[2] * 5;

    UInt64 lo0 = a0, lo1 = a1, lo2 = a2 & 3, lo3 = 0;
    c = Z7_ADDCARRY_U64(0, lo0, h5_0, &lo0);
    c = Z7_ADDCARRY_U64(c, lo1, h5_0_hi, &lo1);
    c = Z7_ADDCARRY_U64(c, lo2, 0, &lo2);
    c = Z7_ADDCARRY_U64(0, lo1, h5_1, &lo1);
    c = Z7_ADDCARRY_U64(c, lo2, h5_1_hi, &lo2);
    c = Z7_ADDCARRY_U64(c, lo3, 0, &lo3);
    c = Z7_ADDCARRY_U64(0, lo2, h5_2, &lo2);
    c = Z7_ADDCARRY_U64(c, lo3, 0, &lo3);

    UInt64 ov0 = lo2 >> 2;
    lo2 &= 3;
    lo3 = 0;

    UInt64 ov5_lo, ov5_hi;
    ov5_lo = _umul128(ov0, 5, &ov5_hi);
    c = Z7_ADDCARRY_U64(0, lo0, ov5_lo, &lo0);
    c = Z7_ADDCARRY_U64(c, lo1, ov5_hi, &lo1);
    c = Z7_ADDCARRY_U64(c, lo2, 0, &lo2);

    ov0 = lo2 >> 2;
    if (ov0)
    {
      lo2 &= 3;
      ov5_lo = _umul128(ov0, 5, &ov5_hi);
      c = Z7_ADDCARRY_U64(0, lo0, ov5_lo, &lo0);
      c = Z7_ADDCARRY_U64(c, lo1, ov5_hi, &lo1);
      c = Z7_ADDCARRY_U64(c, lo2, 0, &lo2);
    }

    UInt64 limb0 = lo0 & 0x3FFFFFF;
    UInt64 limb1 = (lo0 >> 26) & 0x3FFFFFF;
    UInt64 limb2 = ((lo0 >> 52) | ((lo1 & 0x3FFF) << 12)) & 0x3FFFFFF;
    UInt64 limb3 = (lo1 >> 14) & 0x3FFFFFF;
    UInt64 limb4 = ((lo1 >> 40) | ((UInt64)lo2 << 24)) & 0x3FFFFFF;

    SetUi32(h, (UInt32)(limb0 | (limb1 << 26)));
    SetUi32(h + 4, (UInt32)((limb1 >> 6) | (limb2 << 20)));
    SetUi32(h + 8, (UInt32)((limb2 >> 12) | (limb3 << 14)));
    SetUi32(h + 12, (UInt32)((limb3 >> 18) | (limb4 << 8)));
  }
#endif
}
#else
#define Poly1305_ProcessBlock_128 Poly1305_ProcessBlock_32
#endif

#ifndef Z7_POLY1305_128BIT

static void Poly1305_ReduceAndPack(Byte h[16], UInt64 m[8])
{
  UInt64 c;

  c = m[0] >> 26; m[0] &= 0x3FFFFFF;
  m[1] += c;
  c = m[1] >> 26; m[1] &= 0x3FFFFFF;
  m[2] += c; c = m[2] >> 26; m[2] &= 0x3FFFFFF;
  m[3] += c; c = m[3] >> 26; m[3] &= 0x3FFFFFF;
  m[4] += c; c = m[4] >> 26; m[4] &= 0x3FFFFFF;
  m[5] += c; c = m[5] >> 26; m[5] &= 0x3FFFFFF;
  m[6] += c; c = m[6] >> 26; m[6] &= 0x3FFFFFF;
  m[7] += c;

  c = (m[3] >> 26); m[3] &= 0x3FFFFFF;
  m[4] += c;

  m[0] += (m[4] >> 26) * 5; m[4] &= 0x3FFFFFF;
  m[1] += (m[5] >> 26) * 5; m[5] &= 0x3FFFFFF;
  m[2] += (m[6] >> 26) * 5; m[6] &= 0x3FFFFFF;
  m[3] += (m[7] >> 26) * 5; m[7] &= 0x3FFFFFF;

  c = m[0] >> 26; m[0] &= 0x3FFFFFF;
  m[1] += c;
  c = m[1] >> 26; m[1] &= 0x3FFFFFF;
  m[2] += c; c = m[2] >> 26; m[2] &= 0x3FFFFFF;
  m[3] += c; c = m[3] >> 26; m[3] &= 0x3FFFFFF;

  m[0] += (m[3] >> 26) * 5; m[3] &= 0x3FFFFFF;

  c = m[0] >> 26; m[0] &= 0x3FFFFFF;
  m[1] += c;

  SetUi32(h, (UInt32)((m[0]) | (m[1] << 26)));
  SetUi32(h + 4, (UInt32)((m[1] >> 6) | (m[2] << 20)));
  SetUi32(h + 8, (UInt32)((m[2] >> 12) | (m[3] << 14)));
  SetUi32(h + 12, (UInt32)((m[3] >> 18) | (m[4] << 8)));
}

#if defined(MY_CPU_X86_OR_AMD64) && defined(MY_CPU_SSE2)

#include <emmintrin.h>

static void Poly1305_ProcessBlock_SSE2_4Way(Byte h[16], const Byte r[16], const Byte block[16], bool hasHighBit)
{
  UInt64 d[4];

  for (unsigned i = 0; i < 3; i++)
    d[i] = (UInt64)GetUi32(h + i * 4);
  d[3] = ((UInt64)GetUi32(h + 12)) & 0x3FFFFFF;

  for (unsigned i = 0; i < 3; i++)
    d[i] += GetUi32(block + i * 4);
  d[3] += ((UInt64)GetUi32(block + 12)) & 0x3FFFFFF;

  if (hasHighBit)
    d[3] |= 0x1000000;

  UInt64 rr[4];
  rr[0] = GetUi32(r) & 0x3FFFFFF;
  rr[1] = ((UInt64)GetUi32(r + 3) >> 2) & 0x3FFFF03;
  rr[2] = ((UInt64)GetUi32(r + 6) >> 4) & 0x3FFC0FF;
  rr[3] = ((UInt64)GetUi32(r + 9) >> 6) & 0x3F03FFF;

  __m128i d_vec = _mm_set_epi32((int)(UInt32)d[3], (int)(UInt32)d[2],
                                 (int)(UInt32)d[1], (int)(UInt32)d[0]);
  __m128i d_swap = _mm_shuffle_epi32(d_vec, _MM_SHUFFLE(0, 3, 0, 1));

  __m128i r_even = _mm_set_epi32(0, (int)(UInt32)rr[2], 0, (int)(UInt32)rr[0]);
  __m128i r_odd  = _mm_set_epi32(0, (int)(UInt32)rr[3], 0, (int)(UInt32)rr[1]);
  __m128i r_cross1 = _mm_set_epi32(0, (int)(UInt32)rr[0], 0, (int)(UInt32)rr[2]);
  __m128i r_cross2 = _mm_set_epi32(0, (int)(UInt32)rr[1], 0, (int)(UInt32)rr[3]);

  UInt64 m[8] = { 0 };
  __m128i prod;
  UInt64 pLo, pHi;

  #define POLY1305_SSE2_MUL_ACC(d_op, r_op, off_lo, off_hi) \
    prod = _mm_mul_epu32(d_op, r_op); \
    _mm_storel_epi64((__m128i *)&pLo, prod); \
    _mm_storel_epi64((__m128i *)&pHi, _mm_srli_si128(prod, 8)); \
    m[off_lo] += pLo; \
    m[off_hi] += pHi;

  POLY1305_SSE2_MUL_ACC(d_vec,  r_even,   0, 4)
  POLY1305_SSE2_MUL_ACC(d_vec,  r_odd,    1, 5)
  POLY1305_SSE2_MUL_ACC(d_swap, r_even,   1, 5)
  POLY1305_SSE2_MUL_ACC(d_swap, r_odd,    2, 6)
  POLY1305_SSE2_MUL_ACC(d_vec,  r_cross1, 2, 2)
  POLY1305_SSE2_MUL_ACC(d_vec,  r_cross2, 3, 3)
  POLY1305_SSE2_MUL_ACC(d_swap, r_cross1, 3, 3)
  POLY1305_SSE2_MUL_ACC(d_swap, r_cross2, 4, 4)

  #undef POLY1305_SSE2_MUL_ACC

  Poly1305_ReduceAndPack(h, m);
}

#endif

#if !defined(MY_CPU_X86_OR_AMD64) || !defined(MY_CPU_SSE2)
static void Poly1305_ProcessBlock_32(Byte h[16], const Byte r[16], const Byte block[16], bool hasHighBit)
{
  UInt64 d[8] = { 0 };

  for (unsigned i = 0; i < 3; i++)
  {
    d[i] = (UInt64)GetUi32(h + i * 4);
  }
  d[3] = ((UInt64)GetUi32(h + 12)) & 0x3FFFFFF;

  for (unsigned i = 0; i < 3; i++)
  {
    UInt64 t = GetUi32(block + i * 4);
    d[i] += t;
  }
  d[3] += ((UInt64)GetUi32(block + 12)) & 0x3FFFFFF;

  if (hasHighBit)
    d[3] |= 0x1000000;

  UInt64 rr[4];
  rr[0] = GetUi32(r) & 0x3FFFFFF;
  rr[1] = ((UInt64)GetUi32(r + 3) >> 2) & 0x3FFFF03;
  rr[2] = ((UInt64)GetUi32(r + 6) >> 4) & 0x3FFC0FF;
  rr[3] = ((UInt64)GetUi32(r + 9) >> 6) & 0x3F03FFF;

  UInt64 m[8] = { 0 };
  for (unsigned i = 0; i < 4; i++)
  {
    for (unsigned j = 0; j < 4; j++)
    {
      m[i + j] += d[i] * rr[j];
    }
  }

  Poly1305_ReduceAndPack(h, m);
}
#endif
#endif

#ifdef Z7_POLY1305_128BIT
static void Poly1305_ProcessBlock(Byte h[16], const Byte r[16], const Byte block[16], bool hasHighBit)
{
  Poly1305_ProcessBlock_128(h, r, block, hasHighBit);
}
#else
static void Poly1305_ProcessBlock(Byte h[16], const Byte r[16], const Byte block[16], bool hasHighBit)
{
#if defined(MY_CPU_X86_OR_AMD64) && defined(MY_CPU_SSE2)
  Poly1305_ProcessBlock_SSE2_4Way(h, r, block, hasHighBit);
#else
  Poly1305_ProcessBlock_32(h, r, block, hasHighBit);
#endif
}
#endif

void CPoly1305::ProcessBlocks(Byte *buf, unsigned &bufPos, UInt64 &len, const Byte *data, UInt32 size)
{
  len += size;

  if (bufPos > 0)
  {
    unsigned n = 16 - bufPos;
    if (n > size) n = size;
    memcpy(buf + bufPos, data, n);
    bufPos += n;
    data += n;
    size -= n;
    if (bufPos == 16)
    {
      Poly1305_ProcessBlock(_h, _r, buf, true);
      bufPos = 0;
    }
  }

  while (size >= 16)
  {
    Poly1305_ProcessBlock(_h, _r, data, true);
    data += 16;
    size -= 16;
  }

  if (size > 0)
  {
    memcpy(buf, data, size);
    bufPos = size;
  }
}

void CPoly1305::Update(const Byte *data, UInt32 size)
{
  if (_finalized) return;
  ProcessBlocks(_block, _blockPos, _totalLen, data, size);
}

void CPoly1305::UpdateAad(const Byte *data, UInt32 size)
{
  if (_finalized) return;
  ProcessBlocks(_aadBlock, _aadBlockPos, _aadLen, data, size);
}

void CPoly1305::PadAndProcessBlock(Byte *buf, unsigned bufPos, UInt64 len)
{
  unsigned mod = (unsigned)(len & 0xF);
  if (mod != 0)
  {
    unsigned padLen = 16 - mod;
    memset(buf + bufPos, 0, padLen);
    Poly1305_ProcessBlock(_h, _r, buf, true);
  }
}

void CPoly1305::Final(Byte *tag)
{
  if (_finalized)
    return;
  _finalized = true;

  PadAndProcessBlock(_aadBlock, _aadBlockPos, _aadLen);
  PadAndProcessBlock(_block, _blockPos, _totalLen);

  {
    Byte lenBlock[16];
    for (unsigned i = 0; i < 8; i++)
      lenBlock[i] = (Byte)(_aadLen >> (i * 8));
    for (unsigned i = 0; i < 8; i++)
      lenBlock[8 + i] = (Byte)(_totalLen >> (i * 8));
    Poly1305_ProcessBlock(_h, _r, lenBlock, true);
  }

  UInt64 h0 = (UInt64)GetUi32(_h);
  UInt64 h1 = (UInt64)GetUi32(_h + 4);
  UInt64 h2 = (UInt64)GetUi32(_h + 8);
  UInt64 h3 = (UInt64)GetUi32(_h + 12) & 0x3FFFFFF;

  UInt64 s0 = (UInt64)GetUi32(_s);
  UInt64 s1 = (UInt64)GetUi32(_s + 4);
  UInt64 s2 = (UInt64)GetUi32(_s + 8);
  UInt64 s3 = (UInt64)GetUi32(_s + 12);

  h0 += s0;
  UInt64 c = h0 >> 26; h0 &= 0x3FFFFFF;
  h1 += s1 + c; c = h1 >> 26; h1 &= 0x3FFFFFF;
  h2 += s2 + c; c = h2 >> 26; h2 &= 0x3FFFFFF;
  h3 += s3 + c;

  UInt64 g0, g1, g2, g3;
  g0 = h0 + 5;
  c = g0 >> 26; g0 &= 0x3FFFFFF;
  g1 = h1 + c; c = g1 >> 26; g1 &= 0x3FFFFFF;
  g2 = h2 + c; c = g2 >> 26; g2 &= 0x3FFFFFF;
  g3 = h3 + c - 4;

  UInt64 mask = (g3 >> 63) - 1;
  h0 = (h0 & ~mask) | (g0 & mask);
  h1 = (h1 & ~mask) | (g1 & mask);
  h2 = (h2 & ~mask) | (g2 & mask);
  h3 = (h3 & ~mask) | (g3 & mask);

  SetUi32(tag, (UInt32)(h0 | (h1 << 26)));
  SetUi32(tag + 4, (UInt32)((h1 >> 6) | (h2 << 20)));
  SetUi32(tag + 8, (UInt32)((h2 >> 12) | (h3 << 14)));
  SetUi32(tag + 12, (UInt32)(h3 >> 18));
}

void CBaseCoder::DeriveKey()
{
  NXChaCha20::XHChaCha20Block_Core(_derivedKey, _key.Key, _nonce);
  ComputePolyKey();
  _poly1305.SetKey(_polyKey);
  if (_aadSize > 0)
  {
    _poly1305.UpdateAad(_aad, _aadSize);
  }
  _derivedKeyValid = true;
}

Z7_COM7F_IMF(CBaseCoder::Init())
{
  COM_TRY_BEGIN

  PrepareKey();
  _counter = 1;
  _blockPos = kBlockSize;
  _derivedKeyValid = false;
  _poly1305.Reset();

  return S_OK;

  COM_TRY_END
}

#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CEncoder::ResetInitVector())
{
  for (unsigned i = 0; i < sizeof(_nonce); i++)
    _nonce[i] = 0;
  MY_RAND_GEN(_nonce, kNonceSize);
  _counter = 1;
  _blockPos = kBlockSize;
  _derivedKeyValid = false;
  _poly1305.Reset();

  _aadSize = 1;
  const unsigned nonceSizeMinus1 = kNonceSize - 1;
  const unsigned nonceHigh = (nonceSizeMinus1 >= 16) ? (1 << 6) : 0;
  const unsigned nonceLow = nonceSizeMinus1 & 0x0F;
  _aad[0] = (Byte)(_key.NumCyclesPower
      | (_key.SaltSize == 0 ? 0 : (1 << 7))
      | nonceHigh);
  if (_key.SaltSize != 0)
  {
    _aad[1] = (Byte)(((_key.SaltSize - 1) << 4) | nonceLow);
    memcpy(_aad + 2, _key.Salt, _key.SaltSize);
    _aadSize = 2 + _key.SaltSize;
    memcpy(_aad + _aadSize, _nonce, kNonceSize);
    _aadSize += kNonceSize;
  }
  else
  {
    _aad[1] = (Byte)(nonceLow);
    _aadSize = 2;
    memcpy(_aad + _aadSize, _nonce, kNonceSize);
    _aadSize += kNonceSize;
  }

  _tagReady = false;
  memset(_computedTag, 0, kTagSize);
  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CEncoder::Filter(Byte *data, UInt32 size))
{
  ProcessData(data, size);
  _poly1305.Update(data, size);
  return size;
}

Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[2 + sizeof(_key.Salt) + kNonceSize + kTagSize];
  unsigned propsSize = 1;

  const unsigned nonceSizeMinus1 = kNonceSize - 1;
  const unsigned nonceHigh = (nonceSizeMinus1 >= 16) ? (1 << 6) : 0;
  const unsigned nonceLow = nonceSizeMinus1 & 0x0F;

  props[0] = (Byte)(_key.NumCyclesPower
      | (_key.SaltSize == 0 ? 0 : (1 << 7))
      | nonceHigh);

  if (_key.SaltSize != 0)
  {
    props[1] = (Byte)(
        ((_key.SaltSize - 1) << 4)
        | nonceLow);
    memcpy(props + 2, _key.Salt, _key.SaltSize);
    propsSize = 2 + _key.SaltSize;
    memcpy(props + propsSize, _nonce, kNonceSize);
    propsSize += kNonceSize;
  }
  else
  {
    props[1] = (Byte)(nonceLow);
    propsSize = 2;
    memcpy(props + propsSize, _nonce, kNonceSize);
    propsSize += kNonceSize;
  }

  if (!_tagReady)
  {
    _poly1305.Final(_computedTag);
    _tagReady = true;
  }

  memcpy(props + propsSize, _computedTag, kTagSize);
  propsSize += kTagSize;

  return WriteStream(outStream, props, propsSize);
}

CEncoder::CEncoder()
{
  _key.NumCyclesPower = 19;
  _counter = 1;
  _blockPos = kBlockSize;
  _derivedKeyValid = false;
  _aadSize = 0;
  _tagReady = false;
  memset(_computedTag, 0, kTagSize);
}

#endif

CDecoder::CDecoder()
{
  _counter = 1;
  _blockPos = kBlockSize;
  _derivedKeyValid = false;
  _aadSize = 0;
  _authChecked = false;
  _authResult = 0;
  memset(_expectedTag, 0, kTagSize);
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  if (!_derivedKeyValid)
    DeriveKey();
  _poly1305.Update(data, size);
  ProcessData(data, size);
  return size;
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  _key.ClearProps();

  _counter = 1;
  _blockPos = kBlockSize;
  _derivedKeyValid = false;
  _poly1305.Reset();
  _authChecked = false;
  _authResult = 0;
  memset(_expectedTag, 0, kTagSize);

  for (unsigned i = 0; i < sizeof(_nonce); i++)
    _nonce[i] = 0;

  if (size == 0)
    return S_OK;

  const unsigned b0 = data[0];
  _key.NumCyclesPower = b0 & 0x3F;
  if ((b0 & 0xC0) == 0)
    return size == 1 ? S_OK : E_INVALIDARG;
  if (size <= 1)
    return E_INVALIDARG;

  const unsigned b1 = data[1];
  const unsigned saltSize = ((b0 >> 7) & 1) + (b1 >> 4);
  const unsigned nonceSizeMinus1 = ((b0 >> 6) & 1) * 16 + (b1 & 0x0F);
  const unsigned nonceSize = nonceSizeMinus1 + 1;

  const unsigned totalSize = 2 + saltSize + nonceSize + kTagSize;

  if (size != totalSize)
  {
    return E_INVALIDARG;
  }

  _aadSize = totalSize - kTagSize;
  memcpy(_aad, data, _aadSize);

  _key.SaltSize = saltSize;
  data += 2;
  for (unsigned i = 0; i < saltSize; i++)
    _key.Salt[i] = *data++;
  for (unsigned i = 0; i < nonceSize && i < kNonceSize; i++)
    _nonce[i] = *data++;

  memcpy(_expectedTag, data, kTagSize);

  return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX
      || _key.NumCyclesPower == 0x3F) ? S_OK : E_NOTIMPL;
}

Z7_COM7F_IMF(CDecoder::CryptoAuthVerify(Int32 *result))
{
  if (_authChecked)
  {
    *result = _authResult;
    return S_OK;
  }
  _authChecked = true;

  Byte computedTag[kTagSize];
  _poly1305.Final(computedTag);

  {
    volatile Byte diff = 0;
    for (unsigned i = 0; i < kTagSize; i++)
      diff |= computedTag[i] ^ _expectedTag[i];
    _authResult = (diff == 0) ? 0 : 1;
  }
  *result = _authResult;

  Z7_memset_0_ARRAY(computedTag);
  return S_OK;
}

}}