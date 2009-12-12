// Crypto/WzAes.cpp
/*
This code implements Brian Gladman's scheme
specified in password Based File Encryption Utility.

Note: you must include MyAes.cpp to project to initialize AES tables
*/

#include "StdAfx.h"

#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "Pbkdf2HmacSha1.h"
#include "RandGen.h"
#include "WzAes.h"

// define it if you don't want to use speed-optimized version of Pbkdf2HmacSha1
// #define _NO_WZAES_OPTIMIZATIONS

namespace NCrypto {
namespace NWzAes {

const unsigned kAesKeySizeMax = 32;

static const UInt32 kNumKeyGenIterations = 1000;

STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  if(size > kPasswordSizeMax)
    return E_INVALIDARG;
  _key.Password.SetCapacity(size);
  memcpy(_key.Password, data, size);
  return S_OK;
}

#ifndef _NO_WZAES_OPTIMIZATIONS

static void BytesToBeUInt32s(const Byte *src, UInt32 *dest, unsigned destSize)
{
  for (unsigned i = 0; i < destSize; i++)
      dest[i] =
          ((UInt32)(src[i * 4 + 0]) << 24) |
          ((UInt32)(src[i * 4 + 1]) << 16) |
          ((UInt32)(src[i * 4 + 2]) <<  8) |
          ((UInt32)(src[i * 4 + 3]));
}

#endif

STDMETHODIMP CBaseCoder::Init()
{
  UInt32 keySize = _key.GetKeySize();
  UInt32 keysTotalSize = 2 * keySize + kPwdVerifCodeSize;
  Byte buf[2 * kAesKeySizeMax + kPwdVerifCodeSize];
  
  // for (unsigned ii = 0; ii < 1000; ii++)
  {
    #ifdef _NO_WZAES_OPTIMIZATIONS

    NSha1::Pbkdf2Hmac(
      _key.Password, _key.Password.GetCapacity(),
      _key.Salt, _key.GetSaltSize(),
      kNumKeyGenIterations,
      buf, keysTotalSize);

    #else

    UInt32 buf32[(2 * kAesKeySizeMax + kPwdVerifCodeSize + 3) / 4];
    UInt32 key32SizeTotal = (keysTotalSize + 3) / 4;
    UInt32 salt[kSaltSizeMax * 4];
    UInt32 saltSizeInWords = _key.GetSaltSize() / 4;
    BytesToBeUInt32s(_key.Salt, salt, saltSizeInWords);
    NSha1::Pbkdf2Hmac32(
      _key.Password, _key.Password.GetCapacity(),
      salt, saltSizeInWords,
      kNumKeyGenIterations,
      buf32, key32SizeTotal);
    for (UInt32 j = 0; j < keysTotalSize; j++)
      buf[j] = (Byte)(buf32[j / 4] >> (24 - 8 * (j & 3)));
    
    #endif
  }

  _hmac.SetKey(buf + keySize, keySize);
  memcpy(_key.PwdVerifComputed, buf + 2 * keySize, kPwdVerifCodeSize);

  AesCtr2_Init(&_aes);
  Aes_SetKey_Enc(_aes.aes + _aes.offset + 8, buf, keySize);
  return S_OK;
}

HRESULT CEncoder::WriteHeader(ISequentialOutStream *outStream)
{
  UInt32 saltSize = _key.GetSaltSize();
  g_RandomGenerator.Generate(_key.Salt, saltSize);
  Init();
  RINOK(WriteStream(outStream, _key.Salt, saltSize));
  return WriteStream(outStream, _key.PwdVerifComputed, kPwdVerifCodeSize);
}

HRESULT CEncoder::WriteFooter(ISequentialOutStream *outStream)
{
  Byte mac[kMacSize];
  _hmac.Final(mac, kMacSize);
  return WriteStream(outStream, mac, kMacSize);
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size != 1)
    return E_INVALIDARG;
  _key.Init();
  return SetKeyMode(data[0]) ? S_OK : E_INVALIDARG;
}

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  UInt32 saltSize = _key.GetSaltSize();
  UInt32 extraSize = saltSize + kPwdVerifCodeSize;
  Byte temp[kSaltSizeMax + kPwdVerifCodeSize];
  RINOK(ReadStream_FAIL(inStream, temp, extraSize));
  UInt32 i;
  for (i = 0; i < saltSize; i++)
    _key.Salt[i] = temp[i];
  for (i = 0; i < kPwdVerifCodeSize; i++)
    _pwdVerifFromArchive[i] = temp[saltSize + i];
  return S_OK;
}

static bool CompareArrays(const Byte *p1, const Byte *p2, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
    if (p1[i] != p2[i])
      return false;
  return true;
}

bool CDecoder::CheckPasswordVerifyCode()
{
  return CompareArrays(_key.PwdVerifComputed, _pwdVerifFromArchive, kPwdVerifCodeSize);
}

HRESULT CDecoder::CheckMac(ISequentialInStream *inStream, bool &isOK)
{
  isOK = false;
  Byte mac1[kMacSize];
  RINOK(ReadStream_FAIL(inStream, mac1, kMacSize));
  Byte mac2[kMacSize];
  _hmac.Final(mac2, kMacSize);
  isOK = CompareArrays(mac1, mac2, kMacSize);
  return S_OK;
}

CAesCtr2::CAesCtr2()
{
  offset = ((0 - (unsigned)(ptrdiff_t)aes) & 0xF) / sizeof(UInt32);
}

void AesCtr2_Init(CAesCtr2 *p)
{
  UInt32 *ctr = p->aes + p->offset + 4;
  unsigned i;
  for (i = 0; i < 4; i++)
    ctr[i] = 0;
  p->pos = AES_BLOCK_SIZE;
}

void AesCtr2_Code(CAesCtr2 *p, Byte *data, SizeT size)
{
  unsigned pos = p->pos;
  UInt32 *buf32 = p->aes + p->offset;
  if (size == 0)
    return;
  if (pos != AES_BLOCK_SIZE)
  {
    const Byte *buf = (const Byte *)buf32;
    do
      *data++ ^= buf[pos++];
    while (--size != 0 && pos != AES_BLOCK_SIZE);
  }
  if (size >= 16)
  {
    SizeT size2 = size >> 4;
    g_AesCtr_Code(buf32 + 4, data, size2);
    size2 <<= 4;
    data += size2;
    size -= size2;
    pos = AES_BLOCK_SIZE;
  }
  if (size != 0)
  {
    unsigned j;
    const Byte *buf;
    for (j = 0; j < 4; j++)
      buf32[j] = 0;
    g_AesCtr_Code(buf32 + 4, (Byte *)buf32, 1);
    buf = (const Byte *)buf32;
    pos = 0;
    do
      *data++ ^= buf[pos++];
    while (--size != 0 && pos != AES_BLOCK_SIZE);
  }
  p->pos = pos;
}

STDMETHODIMP_(UInt32) CEncoder::Filter(Byte *data, UInt32 size)
{
  AesCtr2_Code(&_aes, data, size);
  _hmac.Update(data, size);
  return size;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  _hmac.Update(data, size);
  AesCtr2_Code(&_aes, data, size);
  return size;
}

}}
