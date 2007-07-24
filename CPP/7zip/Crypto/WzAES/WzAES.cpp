// WzAES.cpp
/*
This code implements Brian Gladman's scheme 
specified in password Based File Encryption Utility.

Note: you must include Crypto/AES/MyAES.cpp to project to initialize AES tables
*/

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"
#include "../Hash/Pbkdf2HmacSha1.h"
#include "../Hash/RandGen.h"

#include "WzAES.h"

// define it if you don't want to use speed-optimized version of Pbkdf2HmacSha1
// #define _NO_WZAES_OPTIMIZATIONS

namespace NCrypto {
namespace NWzAES {

const unsigned int kAesKeySizeMax = 32;

static const UInt32 kNumKeyGenIterations = 1000;

STDMETHODIMP CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  if(size > kPasswordSizeMax)
    return E_INVALIDARG;
  _key.Password.SetCapacity(size);
  memcpy(_key.Password, data, size);
  return S_OK;
}

#define SetUi32(p, d) { UInt32 x = (d); (p)[0] = (Byte)x; (p)[1] = (Byte)(x >> 8); \
    (p)[2] = (Byte)(x >> 16); (p)[3] = (Byte)(x >> 24); }

void CBaseCoder::EncryptData(Byte *data, UInt32 size)
{   
  unsigned int pos = _blockPos;
  for (; size > 0; size--)
  {
    if (pos == AES_BLOCK_SIZE)
    {   
      if (++_counter[0] == 0)
        _counter[1]++;
      UInt32 outBuf[4];
      AesEncode32(_counter, outBuf, Aes.rkey, Aes.numRounds2);
      SetUi32(_buffer,      outBuf[0]);
      SetUi32(_buffer + 4,  outBuf[1]);
      SetUi32(_buffer + 8,  outBuf[2]);
      SetUi32(_buffer + 12, outBuf[3]);
      pos = 0;
    }
    *data++ ^= _buffer[pos++];
  }
  _blockPos = pos;
}

#ifndef _NO_WZAES_OPTIMIZATIONS

static void BytesToBeUInt32s(const Byte *src, UInt32 *dest, int destSize)
{
  for (int i = 0 ; i < destSize; i++)
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
  
  // for (int ii = 0; ii < 1000; ii++)
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
  
  _blockPos = AES_BLOCK_SIZE;
  for (int i = 0; i < 4; i++)
    _counter[i] = 0;

  AesSetKeyEncode(&Aes, buf, keySize);
  return S_OK;
}

static HRESULT SafeWrite(ISequentialOutStream *outStream, const Byte *data, UInt32 size)
{
  UInt32 processedSize;
  RINOK(WriteStream(outStream, data, size, &processedSize));
  return ((processedSize == size) ? S_OK : E_FAIL);
}

/*
STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  Byte keySizeMode = 3;
  return outStream->Write(&keySizeMode, 1, NULL);
}
*/

HRESULT CEncoder::WriteHeader(ISequentialOutStream *outStream)
{
  UInt32 saltSize = _key.GetSaltSize();
  g_RandomGenerator.Generate(_key.Salt, saltSize);
  Init();
  RINOK(SafeWrite(outStream, _key.Salt, saltSize));
  return SafeWrite(outStream, _key.PwdVerifComputed, kPwdVerifCodeSize);
}

HRESULT CEncoder::WriteFooter(ISequentialOutStream *outStream)
{
  Byte mac[kMacSize];
  _hmac.Final(mac, kMacSize);
  return SafeWrite(outStream, mac, kMacSize);
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size != 1)
    return E_INVALIDARG;
  _key.Init();
  Byte keySizeMode = data[0];
  if (keySizeMode < 1 || keySizeMode > 3)
    return E_INVALIDARG;
  _key.KeySizeMode = keySizeMode;
  return S_OK;
}

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  UInt32 saltSize = _key.GetSaltSize();
  UInt32 extraSize = saltSize + kPwdVerifCodeSize;
  Byte temp[kSaltSizeMax + kPwdVerifCodeSize];
  UInt32 processedSize;
  RINOK(ReadStream(inStream, temp, extraSize, &processedSize));
  if (processedSize != extraSize)
    return E_FAIL;
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
  UInt32 processedSize;
  Byte mac1[kMacSize];
  RINOK(ReadStream(inStream, mac1, kMacSize, &processedSize));
  if (processedSize != kMacSize)
    return E_FAIL;
  Byte mac2[kMacSize];
  _hmac.Final(mac2, kMacSize);
  isOK = CompareArrays(mac1, mac2, kMacSize);
  return S_OK;
}

STDMETHODIMP_(UInt32) CEncoder::Filter(Byte *data, UInt32 size)
{
  EncryptData(data, size);
  _hmac.Update(data, size);
  return size;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  _hmac.Update(data, size);
  EncryptData(data, size);
  return size;
}

}}
