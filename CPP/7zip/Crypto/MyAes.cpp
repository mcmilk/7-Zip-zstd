// Crypto/MyAes.cpp

#include "StdAfx.h"

#include "MyAes.h"

namespace NCrypto {

struct CAesTabInit { CAesTabInit() { AesGenTables();} } g_AesTabInit;

CAesCbcCoder::CAesCbcCoder()
{
  _offset = ((0 - (unsigned)(ptrdiff_t)_aes) & 0xF) / sizeof(UInt32);
}

STDMETHODIMP CAesCbcCoder::Init() { return S_OK; }

STDMETHODIMP_(UInt32) CAesCbcCoder::Filter(Byte *data, UInt32 size)
{
  if (size == 0)
    return 0;
  if (size < AES_BLOCK_SIZE)
    return AES_BLOCK_SIZE;
  size >>= 4;
  _codeFunc(_aes + _offset, data, size);
  return size << 4;
}

STDMETHODIMP CAesCbcCoder::SetKey(const Byte *data, UInt32 size)
{
  if ((size & 0x7) != 0 || size < 16 || size > 32)
    return E_INVALIDARG;
  _setKeyFunc(_aes + _offset + 4, data, size);
  return S_OK;
}

STDMETHODIMP CAesCbcCoder::SetInitVector(const Byte *data, UInt32 size)
{
  if (size != AES_BLOCK_SIZE)
    return E_INVALIDARG;
  AesCbc_Init(_aes + _offset, data);
  return S_OK;
}

CAesCbcEncoder::CAesCbcEncoder() { _codeFunc = g_AesCbc_Encode; _setKeyFunc = Aes_SetKey_Enc; }
CAesCbcDecoder::CAesCbcDecoder() { _codeFunc = g_AesCbc_Decode; _setKeyFunc = Aes_SetKey_Dec; }

}
