// Cascade.cpp
// Copyright (C) fzxx   Contributor: https://github.com/fzxx
// License: GNU LGPL v2.1+

#include "StdAfx.h"

#include "../../../C/CpuArch.h"
#include "../../../C/Aes.h"

#include "../../Common/ComTry.h"

#ifndef Z7_ST
#include "../../Windows/Synchronization.h"
#endif

#include "../Common/StreamUtils.h"

#include "Cascade.h"
#include "HkdfBlake2sp.h"
#include "XChaCha20.h"
#include "AsconSimd.h"

#ifndef Z7_EXTRACT_ONLY
#include "RandGen.h"
#endif

namespace NCrypto {

static void XorBytes(Byte *dst, const Byte *src, unsigned len)
{
  Byte *d = dst;
  const Byte *s = src;

#ifdef MY_CPU_LE_UNALIGN_64
  while (len >= 8)
  {
    *(UInt64 *)d ^= *(const UInt64 *)s;
    d += 8;
    s += 8;
    len -= 8;
  }
#endif

#ifdef MY_CPU_LE_UNALIGN
  while (len >= 4)
  {
    *(UInt32 *)d ^= *(const UInt32 *)s;
    d += 4;
    s += 4;
    len -= 4;
  }
#endif

  while (len--)
    *d++ ^= *s++;
}

namespace NAXPCascade {

static CKeyInfoCache g_AXP_GlobalKeyCache(32);

#ifndef Z7_ST
  static NWindows::NSynchronization::CCriticalSection g_AXP_GlobalKeyCacheCriticalSection;
  #define AXP_MT_LOCK NWindows::NSynchronization::CCriticalSectionLock lock(g_AXP_GlobalKeyCacheCriticalSection);
#else
  #define AXP_MT_LOCK
#endif

CAXPBase::CAXPBase():
  _cachedKeys(16),
  _keyDerived(false),
  _xcBlockPos(64),
  _xcCounter(0),
  _aadSize(0),
  _finalized(false),
  _authOk(false)
{
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  Z7_memset_0_ARRAY(_keyAes);
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_aesKeys);
  Z7_memset_0_ARRAY(_keyXChaCha20);
  Z7_memset_0_ARRAY(_xcNonce);
  Z7_memset_0_ARRAY(_xcDerivedKey);
  Z7_memset_0_ARRAY(_xcBlock);
  Z7_memset_0_ARRAY(_polyKey);
}

void CAXPBase::PrepareKey()
{
  AXP_MT_LOCK

  bool finded = false;
  if (!_cachedKeys.GetKey(_key))
  {
    finded = g_AXP_GlobalKeyCache.GetKey(_key);
    if (!finded)
      _key.CalcKey();
    _cachedKeys.Add(_key);
  }
  if (!finded)
    g_AXP_GlobalKeyCache.FindAndAdd(_key);
  _keyDerived = false;
}

void CAXPBase::DeriveAXPKeys()
{
  NHkdfBlake2sp::Derive(
      _key.CascadeKey, kCascadeKeySize,
      "AES-key", 7,
      _keyAes, 32);
  Aes_SetKey_Enc(_aesKeys + 4, _keyAes, 32);
  memcpy(_aesKeys, _aesIv, 16);

  NHkdfBlake2sp::Derive(
      _key.CascadeKey, kCascadeKeySize,
      "XChaCha20-key", 13,
      _keyXChaCha20, 32);
  NXChaCha20::XHChaCha20Block_Core(_xcDerivedKey, _keyXChaCha20, _xcNonce);

  ComputePolyKey();
  _poly1305.SetKey(_polyKey);
  if (_aadSize > 0)
    _poly1305.UpdateAad(_aad, _aadSize);

  _xcBlockPos = 64;
  _xcCounter = 1;

  _keyDerived = true;
  _finalized = false;
}

void CAXPBase::ComputePolyKey()
{
  Byte polyBlock[64];
  NXChaCha20::XChaCha20Block_Core(polyBlock, _xcDerivedKey, _xcNonce + 16, 0);
  memcpy(_polyKey, polyBlock, kPolyKeySize);
  Z7_memset_0_ARRAY(polyBlock);
}

void CAXPBase::AesCtrXorData(Byte *data, UInt32 size)
{
  if (size >= AES_BLOCK_SIZE)
  {
    UInt32 numBlocks = size >> 4;
    AesCtr_Code(_aesKeys, data, numBlocks);
    data += numBlocks << 4;
    size -= numBlocks << 4;
  }
  if (size > 0)
  {
    Byte temp[16];
    memset(temp, 0, 16);
    AesCtr_Code(_aesKeys, temp, 1);
    for (UInt32 i = 0; i < size; i++)
      data[i] ^= temp[i];
    Z7_memset_0_ARRAY(temp);
  }
}

void CAXPBase::XChaCha20XorData(Byte *data, UInt32 size)
{
  while (size > 0)
  {
    if (_xcBlockPos >= kXcBlockSize)
    {
      NXChaCha20::XChaCha20Block_Core(_xcBlock, _xcDerivedKey, _xcNonce + 16, _xcCounter);
      _xcBlockPos = 0;
      _xcCounter++;
    }
    UInt32 avail = kXcBlockSize - _xcBlockPos;
    UInt32 toProcess = (size < avail) ? size : avail;
    XorBytes(data, _xcBlock + _xcBlockPos, toProcess);
    data += toProcess;
    size -= toProcess;
    _xcBlockPos += toProcess;
  }
}

void CAXPBaseCoder::ProcessEnc(Byte *data, UInt32 size)
{
  if (!_keyDerived)
    DeriveAXPKeys();

  AesCtrXorData(data, size);
  XChaCha20XorData(data, size);
  _poly1305.Update(data, size);
}

void CAXPBaseCoder::ProcessDec(Byte *data, UInt32 size)
{
  if (!_keyDerived)
    DeriveAXPKeys();

  _poly1305.Update(data, size);
  XChaCha20XorData(data, size);
  AesCtrXorData(data, size);
}

Z7_COM7F_IMF(CAXPBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size))
{
  COM_TRY_BEGIN

  _key.Password.Wipe();
  _key.Password.CopyFrom(data, (size_t)size);
  _keyDerived = false;
  return S_OK;

  COM_TRY_END
}

Z7_COM7F_IMF(CAXPBaseCoder::Init())
{
  COM_TRY_BEGIN

  PrepareKey();
  _keyDerived = false;
  _finalized = false;
  _authOk = false;
  _poly1305.Reset();
  return S_OK;

  COM_TRY_END
}

Z7_COM7F_IMF2(UInt32, CAXPBaseCoder::Filter(Byte * /* data */, UInt32 size))
{
  return size;
}

#ifndef Z7_EXTRACT_ONLY

CAXPEncoder::CAXPEncoder()
{
  _key.NumCyclesPower = 19;
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  _keyDerived = false;
  _aadSize = 0;
  _finalized = false;
  _xcBlockPos = 64;
  _xcCounter = 1;
  _tagReady = false;
  memset(_computedTag, 0, kTagSize);
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_xcNonce);
}

Z7_COM7F_IMF(CAXPEncoder::ResetInitVector())
{
  for (unsigned i = 0; i < sizeof(_aesIv); i++)
    _aesIv[i] = 0;
  for (unsigned i = 0; i < sizeof(_xcNonce); i++)
    _xcNonce[i] = 0;

  MY_RAND_GEN(_key.Salt, N7zKeyDerivation::kCascadeSaltSize);
  _key.SaltSize = N7zKeyDerivation::kCascadeSaltSize;

  MY_RAND_GEN(_aesIv, 16);
  MY_RAND_GEN(_xcNonce, 24);
  _keyDerived = false;
  _finalized = false;
  _xcBlockPos = 64;
  _xcCounter = 1;
  _poly1305.Reset();
  _tagReady = false;
  memset(_computedTag, 0, kTagSize);

  _aadSize = 1;
  _aad[0] = (Byte)(_key.NumCyclesPower
      | (1 << 7)
      | (1 << 6));

  _aad[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
  memcpy(_aad + 2, _key.Salt, _key.SaltSize);
  _aadSize = 2 + _key.SaltSize;
  memcpy(_aad + _aadSize, _aesIv, 16);
  _aadSize += 16;
  memcpy(_aad + _aadSize, _xcNonce, 24);
  _aadSize += 24;

  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CAXPEncoder::Filter(Byte *data, UInt32 size))
{
  if (size == 0)
    return 0;

  ProcessEnc(data, size);
  return size;
}

Z7_COM7F_IMF(CAXPEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[2 + sizeof(_key.Salt) + 24 + 16 + kTagSize];
  unsigned propsSize = 1;

  props[0] = (Byte)(_key.NumCyclesPower
      | (1 << 7)
      | (1 << 6));

  props[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
  memcpy(props + 2, _key.Salt, _key.SaltSize);
  propsSize = 2 + _key.SaltSize;
  memcpy(props + propsSize, _aesIv, 16);
  propsSize += 16;
  memcpy(props + propsSize, _xcNonce, 24);
  propsSize += 24;

  if (!_tagReady)
  {
    if (_finalized)
    {
      _tagReady = true;
    }
    else if (_keyDerived)
    {
      _poly1305.Final(_computedTag);
      _finalized = true;
      _tagReady = true;
    }
    else
    {
      memset(_computedTag, 0, kTagSize);
    }
  }

  memcpy(props + propsSize, _computedTag, kTagSize);
  propsSize += kTagSize;

  return WriteStream(outStream, props, propsSize);
}

#endif

CAXPDecoder::CAXPDecoder()
{
  _key.NumCyclesPower = 19;
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  _keyDerived = false;
  _finalized = false;
  _authOk = false;
  _aadSize = 0;
  memset(_expectedTag, 0, kTagSize);
  _xcBlockPos = 64;
  _xcCounter = 1;
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_xcNonce);
}

Z7_COM7F_IMF2(UInt32, CAXPDecoder::Filter(Byte *data, UInt32 size))
{
  if (size == 0)
    return 0;

  ProcessDec(data, size);
  return size;
}

Z7_COM7F_IMF(CAXPDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  _key.ClearProps();
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;

  _keyDerived = false;
  _finalized = false;
  _authOk = false;
  _poly1305.Reset();
  memset(_expectedTag, 0, kTagSize);
  _xcBlockPos = 64;
  _xcCounter = 1;

  for (unsigned i = 0; i < sizeof(_aesIv); i++)
    _aesIv[i] = 0;
  for (unsigned i = 0; i < sizeof(_xcNonce); i++)
    _xcNonce[i] = 0;

  if (size == 0)
    return S_OK;

  const unsigned b0 = data[0];
  _key.NumCyclesPower = b0 & 0x3F;

  const bool saltPresent = (b0 & 0x80) != 0;
  const unsigned nonceType = (b0 >> 6) & 1;

  if (!saltPresent && nonceType == 0 && size == 1)
    return S_OK;
  if (size <= 1)
    return E_INVALIDARG;

  const unsigned b1 = data[1];
  const unsigned saltSize = saltPresent ? (((b1 >> 3) & 0x1F) + 1) : 0;
  const unsigned nonceSize = (nonceType == 0) ? 16 : 24;

  const unsigned minSize = 2 + saltSize + nonceSize + 16;
  if (size < minSize)
    return E_INVALIDARG;

  const unsigned tagSize = size - minSize;
  if (tagSize != kTagSize && tagSize != 0)
    return E_INVALIDARG;

  _key.SaltSize = saltSize;
  data += 2;
  for (unsigned i = 0; i < saltSize; i++)
    _key.Salt[i] = *data++;
  for (unsigned i = 0; i < 16; i++)
    _aesIv[i] = *data++;
  for (unsigned i = 0; i < nonceSize && i < 24; i++)
    _xcNonce[i] = *data++;

  if (tagSize == kTagSize)
    memcpy(_expectedTag, data, kTagSize);

  _aadSize = 1;
  _aad[0] = (Byte)(_key.NumCyclesPower
      | (saltPresent ? (1 << 7) : 0)
      | (nonceType << 6));

  if (saltPresent)
  {
    _aad[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
    memcpy(_aad + 2, _key.Salt, _key.SaltSize);
    _aadSize = 2 + _key.SaltSize;
    memcpy(_aad + _aadSize, _aesIv, 16);
    _aadSize += 16;
    memcpy(_aad + _aadSize, _xcNonce, nonceSize);
    _aadSize += nonceSize;
  }
  else
  {
    _aad[1] = 0;
    _aadSize = 2;
    memcpy(_aad + _aadSize, _aesIv, 16);
    _aadSize += 16;
    memcpy(_aad + _aadSize, _xcNonce, nonceSize);
    _aadSize += nonceSize;
  }

  return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX
      || _key.NumCyclesPower == 0x3F) ? S_OK : E_NOTIMPL;
}

Z7_COM7F_IMF(CAXPDecoder::CryptoAuthVerify(Int32 *result))
{
  if (_authOk)
  {
    *result = 0;
    return S_OK;
  }

  if (!_keyDerived)
    DeriveAXPKeys();

  Byte computedTag[kTagSize];
  _poly1305.Final(computedTag);
  _finalized = true;

  {
    volatile Byte diff = 0;
    for (unsigned i = 0; i < kTagSize; i++)
      diff |= computedTag[i] ^ _expectedTag[i];
    *result = (diff == 0) ? 0 : 1;
    _authOk = (diff == 0);
  }

  Z7_memset_0_ARRAY(computedTag);

  return S_OK;
}

}}

namespace NCrypto {
namespace NAXACascade {

static CKeyInfoCache g_GlobalKeyCache(32);

#ifndef Z7_ST
  static NWindows::NSynchronization::CCriticalSection g_GlobalKeyCacheCriticalSection;
  #define MT_LOCK NWindows::NSynchronization::CCriticalSectionLock lock(g_GlobalKeyCacheCriticalSection);
#else
  #define MT_LOCK
#endif

#ifdef MY_CPU_X86_OR_AMD64
#define ASCON_USE_SSE2 (NAscon::g_SSE2Enabled)
#else
#define ASCON_USE_SSE2 false
#endif

CBase::CBase():
  _cachedKeys(16),
  _keyDerived(false),
  _xcBlockPos(64),
  _xcCounter(0)
{
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  for (unsigned i = 0; i < sizeof(_nonce); i++)
    _nonce[i] = 0;
  Z7_memset_0_ARRAY(_keyAscon);
  Z7_memset_0_ARRAY(_keyAes);
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_aesKeys);
  Z7_memset_0_ARRAY(_keyXChaCha20);
  Z7_memset_0_ARRAY(_xcNonce);
  Z7_memset_0_ARRAY(_xcDerivedKey);
  Z7_memset_0_ARRAY(_xcBlock);
}

void CBase::PrepareKey()
{
  MT_LOCK

  bool finded = false;
  if (!_cachedKeys.GetKey(_key))
  {
    finded = g_GlobalKeyCache.GetKey(_key);
    if (!finded)
      _key.CalcKey();
    _cachedKeys.Add(_key);
  }
  if (!finded)
    g_GlobalKeyCache.FindAndAdd(_key);
  _keyDerived = false;
}

void CBase::DeriveCascadeKeys()
{

  NHkdfBlake2sp::Derive(
      _key.CascadeKey, kCascadeKeySize,
      "AES-key", 7,
      _keyAes, 32);
  Aes_SetKey_Enc(_aesKeys + 4, _keyAes, 32);
  memcpy(_aesKeys, _aesIv, 16);

  NHkdfBlake2sp::Derive(
      _key.CascadeKey, kCascadeKeySize,
      "XChaCha20-key", 13,
      _keyXChaCha20, 32);
  NXChaCha20::XHChaCha20Block_Core(_xcDerivedKey, _keyXChaCha20, _xcNonce);

  NHkdfBlake2sp::Derive(
      _key.CascadeKey, kCascadeKeySize,
      "Ascon-key", 9,
      _keyAscon, NAscon::kKeySize_Ascon);

  _xcBlockPos = 64;
  _xcCounter = 0;

  _keyDerived = true;
}

void CBase::AesCtrXorData(Byte *data, UInt32 size)
{
  if (size >= AES_BLOCK_SIZE)
  {
    UInt32 numBlocks = size >> 4;
    AesCtr_Code(_aesKeys, data, numBlocks);
    data += numBlocks << 4;
    size -= numBlocks << 4;
  }
  if (size > 0)
  {
    Byte temp[16];
    memset(temp, 0, 16);
    AesCtr_Code(_aesKeys, temp, 1);
    for (UInt32 i = 0; i < size; i++)
      data[i] ^= temp[i];
    Z7_memset_0_ARRAY(temp);
  }
}

void CBase::XChaCha20XorData(Byte *data, UInt32 size)
{
  while (size > 0)
  {
    if (_xcBlockPos >= kXcBlockSize)
    {
      NXChaCha20::XChaCha20Block_Core(_xcBlock, _xcDerivedKey, _xcNonce + 16, _xcCounter);
      _xcBlockPos = 0;
      _xcCounter++;
    }
    UInt32 avail = kXcBlockSize - _xcBlockPos;
    UInt32 toProcess = (size < avail) ? size : avail;
    XorBytes(data, _xcBlock + _xcBlockPos, toProcess);
    data += toProcess;
    size -= toProcess;
    _xcBlockPos += toProcess;
  }
}

void CBaseCoder::InitState()
{
  Z7_memset_0_ARRAY(_state);
  Z7_memset_0_ARRAY(_stateBuf);
  _stateBufPos = 0;
  _finalized = false;
  _authOk = false;
}

void CBaseCoder::ProcessAad(const Byte *aad, UInt64 aadLen)
{
  const UInt64 keyLo = GetUi64(_keyAscon);
  const UInt64 keyHi = GetUi64(_keyAscon + 8);
  const UInt64 nonceLo = GetUi64(_nonce);
  const UInt64 nonceHi = GetUi64(_nonce + 8);

  _state[0] = NAscon::kIV_Ascon;
  _state[1] = keyLo;
  _state[2] = keyHi;
  _state[3] = nonceLo;
  _state[4] = nonceHi;

  NAscon::AsconP12(_state);
  _state[3] ^= keyLo;
  _state[4] ^= keyHi;

  if (aadLen > 0)
  {
    UInt64 remaining = aadLen;
    const Byte *p = aad;
    while (remaining >= NAscon::kRateSize)
    {
      _state[0] ^= GetUi64(p);
      _state[1] ^= GetUi64(p + 8);
      NAscon::AsconP8(_state);
      p += NAscon::kRateSize;
      remaining -= NAscon::kRateSize;
    }
    if (remaining >= 8)
    {
      _state[0] ^= GetUi64(p);
      _state[1] ^= ((UInt64)0x01 << ((remaining - 8) * 8));
      if (remaining > 8)
      {
        UInt64 partial = 0;
        memcpy(&partial, p + 8, (unsigned)(remaining - 8));
        _state[1] ^= partial;
      }
    }
    else
    {
      _state[0] ^= ((UInt64)0x01 << (remaining * 8));
      if (remaining > 0)
      {
        UInt64 partial = 0;
        memcpy(&partial, p, (unsigned)remaining);
        _state[0] ^= partial;
      }
    }
    NAscon::AsconP8(_state);
  }

  _state[4] ^= (UInt64)0x80 << 56;
}

void CBaseCoder::ProcessEnc(Byte *data, UInt32 size)
{
#ifdef MY_CPU_X86_OR_AMD64
  NAscon::InitSIMD();
  const bool useSSE2 = ASCON_USE_SSE2;
#else
  const bool useSSE2 = false;
#endif
  if (!_keyDerived)
  {
    DeriveCascadeKeys();
    ProcessAad(_aad, _aadSize);
  }

  AesCtrXorData(data, size);
  XChaCha20XorData(data, size);

  UInt32 remaining = size;
  Byte *p = data;

  if (_stateBufPos > 0)
  {
    UInt32 avail = NAscon::kRateSize - _stateBufPos;
    UInt32 toProcess = (remaining < avail) ? remaining : avail;

    XorBytes(p, _stateBuf + _stateBufPos, toProcess);
    memcpy(_stateBuf + _stateBufPos, p, toProcess);

    _stateBufPos += toProcess;
    p += toProcess;
    remaining -= toProcess;

    if (_stateBufPos == NAscon::kRateSize)
    {
      _state[0] = GetUi64(_stateBuf);
      _state[1] = GetUi64(_stateBuf + 8);
      NAscon::AsconP8(_state);
      _stateBufPos = 0;
    }
  }

  if (remaining >= NAscon::kRateSize)
  {
#ifdef MY_CPU_SSE2
    if (useSSE2)
    {
      do {
        NAscon::AsconEncBlock_SSE2(_state, p);
        NAscon::AsconP8(_state);
        p += NAscon::kRateSize;
        remaining -= NAscon::kRateSize;
      } while (remaining >= NAscon::kRateSize);
    }
    else
#endif
    {
      do {
        _state[0] ^= GetUi64(p);
        _state[1] ^= GetUi64(p + 8);
        SetUi64(p, _state[0]);
        SetUi64(p + 8, _state[1]);
        NAscon::AsconP8(_state);
        p += NAscon::kRateSize;
        remaining -= NAscon::kRateSize;
      } while (remaining >= NAscon::kRateSize);
    }
  }

  if (remaining > 0)
  {
    SetUi64(_stateBuf, _state[0]);
    SetUi64(_stateBuf + 8, _state[1]);
    XorBytes(p, _stateBuf, remaining);
    memcpy(_stateBuf, p, remaining);
    memcpy(_state, _stateBuf, remaining);
    _stateBufPos = remaining;
  }
}

void CBaseCoder::ProcessDec(Byte *data, UInt32 size)
{
#ifdef MY_CPU_X86_OR_AMD64
  NAscon::InitSIMD();
  const bool useSSE2 = ASCON_USE_SSE2;
#else
  const bool useSSE2 = false;
#endif
  if (!_keyDerived)
  {
    DeriveCascadeKeys();
    ProcessAad(_aad, _aadSize);
  }

  {
    UInt32 remaining = size;
    Byte *p = data;

    if (_stateBufPos > 0)
    {
      UInt32 avail = NAscon::kRateSize - _stateBufPos;
      UInt32 toProcess = (remaining < avail) ? remaining : avail;

      {
        UInt32 off = _stateBufPos;
        UInt32 n = toProcess;
        Byte *pd = p;
#ifdef MY_CPU_LE_UNALIGN_64
        while (n >= 8 && (off & 7) == 0)
        {
          UInt64 ct = GetUi64(pd);
          SetUi64(pd, GetUi64(_stateBuf + off) ^ ct);
          SetUi64(_stateBuf + off, ct);
          pd += 8; off += 8; n -= 8;
        }
#endif
#ifdef MY_CPU_LE_UNALIGN
        while (n >= 4 && (off & 3) == 0)
        {
          UInt32 ct = GetUi32(pd);
          SetUi32(pd, GetUi32(_stateBuf + off) ^ ct);
          SetUi32(_stateBuf + off, ct);
          pd += 4; off += 4; n -= 4;
        }
#endif
        while (n--)
        {
          const Byte c = *pd;
          *pd = _stateBuf[off] ^ c;
          _stateBuf[off] = c;
          pd++; off++;
        }
      }

      memcpy((Byte *)_state + _stateBufPos, _stateBuf + _stateBufPos, toProcess);

      _stateBufPos += toProcess;
      p += toProcess;
      remaining -= toProcess;

      if (_stateBufPos == NAscon::kRateSize)
      {
        NAscon::AsconP8(_state);
        _stateBufPos = 0;
      }
    }

    if (remaining >= NAscon::kRateSize)
    {
#ifdef MY_CPU_SSE2
      if (useSSE2)
      {
        do {
          NAscon::AsconDecBlock_SSE2(_state, p);
          NAscon::AsconP8(_state);
          p += NAscon::kRateSize;
          remaining -= NAscon::kRateSize;
        } while (remaining >= NAscon::kRateSize);
      }
      else
#endif
      {
        do {
          UInt64 c0 = GetUi64(p);
          UInt64 c1 = GetUi64(p + 8);
          SetUi64(p, _state[0] ^ c0);
          SetUi64(p + 8, _state[1] ^ c1);
          _state[0] = c0;
          _state[1] = c1;
          NAscon::AsconP8(_state);
          p += NAscon::kRateSize;
          remaining -= NAscon::kRateSize;
        } while (remaining >= NAscon::kRateSize);
      }
    }

    if (remaining > 0)
    {
      SetUi64(_stateBuf, _state[0]);
      SetUi64(_stateBuf + 8, _state[1]);

      {
        UInt32 n = remaining;
        Byte *pd = p;
        UInt32 off = 0;
#ifdef MY_CPU_LE_UNALIGN_64
        while (n >= 8)
        {
          UInt64 ct = GetUi64(pd);
          SetUi64(pd, GetUi64(_stateBuf + off) ^ ct);
          SetUi64(_stateBuf + off, ct);
          pd += 8; off += 8; n -= 8;
        }
#endif
#ifdef MY_CPU_LE_UNALIGN
        while (n >= 4)
        {
          UInt32 ct = GetUi32(pd);
          SetUi32(pd, GetUi32(_stateBuf + off) ^ ct);
          SetUi32(_stateBuf + off, ct);
          pd += 4; off += 4; n -= 4;
        }
#endif
        while (n--)
        {
          const Byte c = *pd;
          *pd = _stateBuf[off] ^ c;
          _stateBuf[off] = c;
          pd++; off++;
        }
      }

      memcpy(_state, _stateBuf, remaining);
      _stateBufPos = remaining;
    }
  }

  XChaCha20XorData(data, size);
  AesCtrXorData(data, size);
}

void CBaseCoder::Finalize(Byte *tag)
{
  if (!_finalized)
  {
    if (_stateBufPos > 0 && _stateBufPos < NAscon::kRateSize)
    {
      if (_stateBufPos < 8)
        _state[0] ^= ((UInt64)0x01 << (_stateBufPos * 8));
      else
        _state[1] ^= ((UInt64)0x01 << ((_stateBufPos - 8) * 8));
    }
    else
    {
      _state[0] ^= ((UInt64)0x01);
    }

    const UInt64 keyLo = GetUi64(_keyAscon);
    const UInt64 keyHi = GetUi64(_keyAscon + 8);

    _state[2] ^= keyLo;
    _state[3] ^= keyHi;
    NAscon::AsconP12(_state);
    _state[3] ^= keyLo;
    _state[4] ^= keyHi;

    SetUi64(tag, _state[3]);
    SetUi64(tag + 8, _state[4]);

    _finalized = true;
  }
}

Z7_COM7F_IMF(CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size))
{
  COM_TRY_BEGIN

  _key.Password.Wipe();
  _key.Password.CopyFrom(data, (size_t)size);
  _keyDerived = false;
  return S_OK;

  COM_TRY_END
}

Z7_COM7F_IMF(CBaseCoder::Init())
{
  COM_TRY_BEGIN

  PrepareKey();
  InitState();
  _keyDerived = false;
  return S_OK;

  COM_TRY_END
}

Z7_COM7F_IMF2(UInt32, CBaseCoder::Filter(Byte * /* data */, UInt32 size))
{
  return size;
}

#ifndef Z7_EXTRACT_ONLY

Z7_COM7F_IMF(CEncoder::ResetInitVector())
{
  for (unsigned i = 0; i < sizeof(_nonce); i++)
    _nonce[i] = 0;
  for (unsigned i = 0; i < sizeof(_aesIv); i++)
    _aesIv[i] = 0;
  for (unsigned i = 0; i < sizeof(_xcNonce); i++)
    _xcNonce[i] = 0;

  MY_RAND_GEN(_key.Salt, N7zKeyDerivation::kCascadeSaltSize);
  _key.SaltSize = N7zKeyDerivation::kCascadeSaltSize;

  MY_RAND_GEN(_nonce, NAscon::kNonceSize);
  MY_RAND_GEN(_aesIv, 16);
  MY_RAND_GEN(_xcNonce, 24);
  _keyDerived = false;
  _stateBufPos = 0;
  _finalized = false;
  _xcBlockPos = 64;
  _xcCounter = 0;

  const unsigned nonceType = (NAscon::kNonceSize > 16) ? 1 : 0;

  _aadSize = 1;
  _aad[0] = (Byte)(_key.NumCyclesPower
      | (1 << 7)
      | (nonceType << 6));

  _aad[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
  memcpy(_aad + 2, _key.Salt, _key.SaltSize);
  _aadSize = 2 + _key.SaltSize;
  memcpy(_aad + _aadSize, _aesIv, 16);
  _aadSize += 16;
  memcpy(_aad + _aadSize, _xcNonce, 24);
  _aadSize += 24;
  memcpy(_aad + _aadSize, _nonce, NAscon::kNonceSize);
  _aadSize += NAscon::kNonceSize;

  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CEncoder::Filter(Byte *data, UInt32 size))
{
  if (size == 0)
    return 0;

  ProcessEnc(data, size);
  return size;
}

CEncoder::CEncoder()
{
  _key.NumCyclesPower = 19;
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  _keyDerived = false;
  _stateBufPos = 0;
  _finalized = false;
  _aadSize = 0;
  _xcBlockPos = 64;
  _xcCounter = 0;
  _tagReady = false;
  memset(_computedTag, 0, NAscon::kTagSize);
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_xcNonce);
}

Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[2 + sizeof(_key.Salt) + NAscon::kNonceSize + 16 + 24 + NAscon::kTagSize];
  unsigned propsSize = 1;

  const unsigned nonceType = (NAscon::kNonceSize > 16) ? 1 : 0;

  props[0] = (Byte)(_key.NumCyclesPower
      | (1 << 7)
      | (nonceType << 6));

  props[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
  memcpy(props + 2, _key.Salt, _key.SaltSize);
  propsSize = 2 + _key.SaltSize;
  memcpy(props + propsSize, _aesIv, 16);
  propsSize += 16;
  memcpy(props + propsSize, _xcNonce, 24);
  propsSize += 24;
  memcpy(props + propsSize, _nonce, NAscon::kNonceSize);
  propsSize += NAscon::kNonceSize;

  if (!_tagReady)
  {
    if (_finalized)
    {
      _tagReady = true;
    }
    else if (_keyDerived)
    {
      Finalize(_computedTag);
      _tagReady = true;
    }
    else
    {
      memset(_computedTag, 0, NAscon::kTagSize);
    }
  }

  memcpy(props + propsSize, _computedTag, NAscon::kTagSize);
  propsSize += NAscon::kTagSize;

  return WriteStream(outStream, props, propsSize);
}

#endif

CDecoder::CDecoder()
{
  _key.NumCyclesPower = 19;
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;
  _keyDerived = false;
  _stateBufPos = 0;
  _finalized = false;
  _authOk = false;
  _aadSize = 0;
  memset(_expectedTag, 0, NAscon::kTagSize);
  _xcBlockPos = 64;
  _xcCounter = 0;
  Z7_memset_0_ARRAY(_aesIv);
  Z7_memset_0_ARRAY(_xcNonce);
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  if (size == 0)
    return 0;

  ProcessDec(data, size);
  return size;
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  _key.ClearProps();
  _key.DerivMode = N7zKeyDerivation::kDeriv_Cascade;

  _keyDerived = false;
  _stateBufPos = 0;
  _finalized = false;
  _authOk = false;
  memset(_expectedTag, 0, NAscon::kTagSize);
  _xcBlockPos = 64;
  _xcCounter = 0;

  for (unsigned i = 0; i < sizeof(_nonce); i++)
    _nonce[i] = 0;
  for (unsigned i = 0; i < sizeof(_aesIv); i++)
    _aesIv[i] = 0;
  for (unsigned i = 0; i < sizeof(_xcNonce); i++)
    _xcNonce[i] = 0;

  if (size == 0)
    return S_OK;

  const unsigned b0 = data[0];
  _key.NumCyclesPower = b0 & 0x3F;

  const bool saltPresent = (b0 & 0x80) != 0;
  const unsigned nonceType = (b0 >> 6) & 1;

  if (!saltPresent && nonceType == 0 && size == 1)
    return S_OK;
  if (size <= 1)
    return E_INVALIDARG;

  const unsigned b1 = data[1];
  const unsigned saltSize = saltPresent ? (((b1 >> 3) & 0x1F) + 1) : 0;
  const unsigned nonceSize = (nonceType == 0) ? 16 : 24;

  const unsigned minSize = 2 + saltSize + nonceSize + 16 + 24;
  if (size < minSize)
    return E_INVALIDARG;

  const unsigned tagSize = size - minSize;
  if (tagSize != NAscon::kTagSize && tagSize != 0)
    return E_INVALIDARG;

  _key.SaltSize = saltSize;
  data += 2;
  for (unsigned i = 0; i < saltSize; i++)
    _key.Salt[i] = *data++;
  for (unsigned i = 0; i < 16; i++)
    _aesIv[i] = *data++;
  for (unsigned i = 0; i < 24; i++)
    _xcNonce[i] = *data++;
  for (unsigned i = 0; i < nonceSize && i < NAscon::kNonceSize; i++)
    _nonce[i] = *data++;

  if (tagSize == NAscon::kTagSize)
    memcpy(_expectedTag, data, NAscon::kTagSize);

  _aadSize = 1;
  _aad[0] = (Byte)(_key.NumCyclesPower
      | (saltPresent ? (1 << 7) : 0)
      | (nonceType << 6));

  if (saltPresent)
  {
    _aad[1] = (Byte)(((_key.SaltSize - 1) << 3) & 0xF8);
    memcpy(_aad + 2, _key.Salt, _key.SaltSize);
    _aadSize = 2 + _key.SaltSize;
    memcpy(_aad + _aadSize, _aesIv, 16);
    _aadSize += 16;
    memcpy(_aad + _aadSize, _xcNonce, 24);
    _aadSize += 24;
    memcpy(_aad + _aadSize, _nonce, NAscon::kNonceSize);
    _aadSize += NAscon::kNonceSize;
  }
  else
  {
    _aad[1] = 0;
    _aadSize = 2;
    memcpy(_aad + _aadSize, _aesIv, 16);
    _aadSize += 16;
    memcpy(_aad + _aadSize, _xcNonce, 24);
    _aadSize += 24;
    memcpy(_aad + _aadSize, _nonce, NAscon::kNonceSize);
    _aadSize += NAscon::kNonceSize;
  }

  return (_key.NumCyclesPower <= k_NumCyclesPower_Supported_MAX
      || _key.NumCyclesPower == 0x3F) ? S_OK : E_NOTIMPL;
}

Z7_COM7F_IMF(CDecoder::CryptoAuthVerify(Int32 *result))
{
  if (_authOk)
  {
    *result = 0;
    return S_OK;
  }

  if (!_keyDerived)
  {
    DeriveCascadeKeys();
    ProcessAad(_aad, _aadSize);
  }

  Byte computedTag[NAscon::kTagSize];
  Finalize(computedTag);

  {
    volatile Byte diff = 0;
    for (unsigned i = 0; i < NAscon::kTagSize; i++)
      diff |= computedTag[i] ^ _expectedTag[i];
    *result = (diff == 0) ? 0 : 1;
    _authOk = (diff == 0);
  }

  Z7_memset_0_ARRAY(computedTag);

  return S_OK;
}

}}
