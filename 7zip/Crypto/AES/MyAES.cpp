// Crypto/AES/MyAES.cpp

#include "StdAfx.h"

#include "windows.h"

#include "MyAES.h"
#include "Windows/Defs.h"

#include "AES_CBC.h"

static const int kAESBlockSize = 16;

extern "C" 
{ 
#include "aesopt.h" 
}

class CTabInit
{
public:
  CTabInit() { gen_tabs();}
} g_TabInit;

STDMETHODIMP CAESFilter::Init()
{
  return S_OK;
}

STDMETHODIMP_(UInt32) CAESFilter::Filter(Byte *data, UInt32 size)
{
  if (size > 0 && size < kAESBlockSize)
    return kAESBlockSize;
  UInt32 i;
  for (i = 0; i + kAESBlockSize <= size; i += kAESBlockSize)
  {
    Byte outBlock[kAESBlockSize];
    SubFilter(data + i, outBlock);
    for (int j = 0; j < kAESBlockSize; j++)
      data[i + j] = outBlock[j];
  }
  return i;
}

STDMETHODIMP CAESFilter::SetInitVector(const Byte *data, UInt32 size)
{
  if (size != 16)
    return E_INVALIDARG;
  AES.Init(data);
  return S_OK;
}

STDMETHODIMP CAESEncoder::SetKey(const Byte *data, UInt32 size)
{
  if (AES.enc_key(data, size) != aes_good)
    return E_FAIL;
  return S_OK;
}

void CAESEncoder::SubFilter(const Byte *inBlock, Byte *outBlock)
{
  AES.Encode(inBlock, outBlock);
}

STDMETHODIMP CAESDecoder::SetKey(const Byte *data, UInt32 size)
{
  if (AES.dec_key(data, size) != aes_good)
    return E_FAIL;
  return S_OK;
}

void CAESDecoder::SubFilter(const Byte *inBlock, Byte *outBlock)
{
  AES.Decode(inBlock, outBlock);
}
