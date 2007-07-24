// RarAES.cpp
// Note: you must include Crypto/AES/MyAES.cpp to project to initialize AES tables

#include "StdAfx.h"

#include "RarAES.h"
#include "../../Common/MethodId.h"
#include "../Hash/Sha1.h"

namespace NCrypto {
namespace NRar29 {

CDecoder::CDecoder():
  _thereIsSalt(false),
  _needCalculate(true),
  _rar350Mode(false)
{
  for (int i = 0; i < sizeof(_salt); i++)
    _salt[i] = 0;
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  bool thereIsSaltPrev = _thereIsSalt;
  _thereIsSalt = false;
  if (size == 0)
    return S_OK;
  if (size < 8)
    return E_INVALIDARG;
  _thereIsSalt = true;
  bool same = false;
  if (_thereIsSalt == thereIsSaltPrev)
  {
    same = true;
    if (_thereIsSalt)
    {
      for (int i = 0; i < sizeof(_salt); i++)
        if (_salt[i] != data[i])
        {
          same = false;
          break;
        }
    }
  }
  for (int i = 0; i < sizeof(_salt); i++)
    _salt[i] = data[i];
  if (!_needCalculate && !same)
    _needCalculate = true;
  return S_OK;
}

static const int kMaxPasswordLength = 127 * 2;

STDMETHODIMP CDecoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  if (size > kMaxPasswordLength)
    size = kMaxPasswordLength;
  bool same = false;
  if (size == buffer.GetCapacity())
  {
    same = true;
    for (UInt32 i = 0; i < size; i++)
      if (data[i] != buffer[i])
      {
        same = false;
        break;
      }
  }
  if (!_needCalculate && !same)
    _needCalculate = true;
  buffer.SetCapacity(size);
  memcpy(buffer, data, size);
  return S_OK;
}

STDMETHODIMP CDecoder::Init()
{
  Calculate();
  AesSetKeyDecode(&Aes.aes, aesKey, kRarAesKeySize);
  AesCbcInit(&Aes, aesInit);
  return S_OK;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  return AesCbcDecode(&Aes, data, size);
}

void CDecoder::Calculate()
{
  if (_needCalculate)
  {
    const int kSaltSize = 8;
    
    Byte rawPassword[kMaxPasswordLength + kSaltSize];
    
    memcpy(rawPassword, buffer, buffer.GetCapacity());
    
    size_t rawLength = buffer.GetCapacity();
    
    if (_thereIsSalt)
    {
      memcpy(rawPassword + rawLength, _salt, kSaltSize);
      rawLength += kSaltSize;
    }
    
    NSha1::CContext sha;
    sha.Init();

    // seems rar reverts hash for sha.
    const int hashRounds = 0x40000;
    int i;
    for (i = 0; i < hashRounds; i++)
    {
      sha.Update(rawPassword, rawLength, _rar350Mode);
      Byte pswNum[3] = { (Byte)i, (Byte)(i >> 8), (Byte)(i >> 16) };
      sha.Update(pswNum, 3, _rar350Mode);
      if (i % (hashRounds / 16) == 0)
      {
        NSha1::CContext shaTemp = sha;
        Byte digest[NSha1::kDigestSize];
        shaTemp.Final(digest);
        aesInit[i / (hashRounds / 16)] = (Byte)digest[4 * 4 + 3];
      }
    }
    /*
    // it's test message for sha
    const char *message = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    sha.Update((const Byte *)message, strlen(message));
    */
    Byte digest[20];
    sha.Final(digest);
    for (i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        aesKey[i * 4 + j] = (digest[i * 4 + 3 - j]);
  }
  _needCalculate = false;
}

}}
