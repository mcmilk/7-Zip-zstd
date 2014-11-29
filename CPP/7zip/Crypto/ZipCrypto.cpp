// Crypto/ZipCrypto.cpp

#include "StdAfx.h"

#include "../../../C/7zCrc.h"

#include "../Common/StreamUtils.h"

#include "RandGen.h"
#include "ZipCrypto.h"

namespace NCrypto {
namespace NZip {

void CCipher::UpdateKeys(Byte b)
{
  Keys[0] = CRC_UPDATE_BYTE(Keys[0], b);
  Keys[1] = (Keys[1] + (Keys[0] & 0xFF)) * 0x8088405 + 1;
  Keys[2] = CRC_UPDATE_BYTE(Keys[2], (Byte)(Keys[1] >> 24));
}

STDMETHODIMP CCipher::CryptoSetPassword(const Byte *password, UInt32 passwordLen)
{
  Keys[0] = 0x12345678;
  Keys[1] = 0x23456789;
  Keys[2] = 0x34567890;
  UInt32 i;
  for (i = 0; i < passwordLen; i++)
    UpdateKeys(password[i]);
  for (i = 0; i < 3; i++)
    Keys2[i] = Keys[i];
  return S_OK;
}

STDMETHODIMP CCipher::Init()
{
  return S_OK;
}

Byte CCipher::DecryptByteSpec()
{
  UInt32 temp = Keys[2] | 2;
  return (Byte)((temp * (temp ^ 1)) >> 8);
}

HRESULT CEncoder::WriteHeader(ISequentialOutStream *outStream, UInt32 crc)
{
  Byte h[kHeaderSize];
  g_RandomGenerator.Generate(h, kHeaderSize - 2);
  h[kHeaderSize - 1] = (Byte)(crc >> 24);
  h[kHeaderSize - 2] = (Byte)(crc >> 16);
  RestoreKeys();
  Filter(h, kHeaderSize);
  return WriteStream(outStream, h, kHeaderSize);
}

STDMETHODIMP_(UInt32) CEncoder::Filter(Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
  {
    Byte b = data[i];
    data[i] = (Byte)(b ^ DecryptByteSpec());;
    UpdateKeys(b);
  }
  return size;
}

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  Byte h[kHeaderSize];
  RINOK(ReadStream_FAIL(inStream, h, kHeaderSize));
  RestoreKeys();
  Filter(h, kHeaderSize);
  return S_OK;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  for (UInt32 i = 0; i < size; i++)
  {
    Byte c = (Byte)(data[i] ^ DecryptByteSpec());
    UpdateKeys(c);
    data[i] = c;
  }
  return size;
}

}}
