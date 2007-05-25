// Crypto/Rar20Cipher.cpp

#include "StdAfx.h"

#include "Rar20Cipher.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NRar20 {

STDMETHODIMP CDecoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  _coder.SetPassword(data, size);
  return S_OK;
}

STDMETHODIMP CDecoder::Init()
{
  return S_OK;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  const UInt16 kBlockSize = 16;
  if (size > 0 && size < kBlockSize)
    return kBlockSize;
  UInt32 i;
  for (i = 0; i + kBlockSize <= size; i += kBlockSize)
  {
    _coder.DecryptBlock(data + i);
  }
  return i;
}

}}
