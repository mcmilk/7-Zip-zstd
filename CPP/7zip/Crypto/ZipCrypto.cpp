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
  Keys[1] += Keys[0] & 0xff;
  Keys[1] = Keys[1] * 134775813L + 1;
  Keys[2] = CRC_UPDATE_BYTE(Keys[2], (Byte)(Keys[1] >> 24));
}

void CCipher::SetPassword(const Byte *password, UInt32 passwordLen)
{
  Keys[0] = 305419896L;
  Keys[1] = 591751049L;
  Keys[2] = 878082192L;
  for (UInt32 i = 0; i < passwordLen; i++)
    UpdateKeys(password[i]);
}

Byte CCipher::DecryptByteSpec()
{
  UInt32 temp = Keys[2] | 2;
  return (Byte)((temp * (temp ^ 1)) >> 8);
}

Byte CCipher::DecryptByte(Byte b)
{
  Byte c = (Byte)(b ^ DecryptByteSpec());
  UpdateKeys(c);
  return c;
}

Byte CCipher::EncryptByte(Byte b)
{
  Byte c = (Byte)(b ^ DecryptByteSpec());
  UpdateKeys(b);
  return c;
}

void CCipher::DecryptHeader(Byte *buf)
{
  for (unsigned i = 0; i < kHeaderSize; i++)
    buf[i] = DecryptByte(buf[i]);
}

void CCipher::EncryptHeader(Byte *buf)
{
  for (unsigned i = 0; i < kHeaderSize; i++)
    buf[i] = EncryptByte(buf[i]);
}

STDMETHODIMP CEncoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  _cipher.SetPassword(data, size);
  return S_OK;
}

STDMETHODIMP CEncoder::CryptoSetCRC(UInt32 crc)
{
  _crc = crc;
  return S_OK;
}

STDMETHODIMP CEncoder::Init()
{
  return S_OK;
}

HRESULT CEncoder::WriteHeader(ISequentialOutStream *outStream)
{
  Byte header[kHeaderSize];
  g_RandomGenerator.Generate(header, kHeaderSize - 2);

  header[kHeaderSize - 1] = Byte(_crc >> 24);
  header[kHeaderSize - 2] = Byte(_crc >> 16);

  _cipher.EncryptHeader(header);
  return WriteStream(outStream, header, kHeaderSize);
}

STDMETHODIMP_(UInt32) CEncoder::Filter(Byte *data, UInt32 size)
{
  UInt32 i;
  for (i = 0; i < size; i++)
    data[i] = _cipher.EncryptByte(data[i]);
  return i;
}

STDMETHODIMP CDecoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  _cipher.SetPassword(data, size);
  return S_OK;
}

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  Byte header[kHeaderSize];
  RINOK(ReadStream_FAIL(inStream, header, kHeaderSize));
  _cipher.DecryptHeader(header);
  return S_OK;
}

STDMETHODIMP CDecoder::Init()
{
  return S_OK;
}

STDMETHODIMP_(UInt32) CDecoder::Filter(Byte *data, UInt32 size)
{
  UInt32 i;
  for (i = 0; i < size; i++)
    data[i] = _cipher.DecryptByte(data[i]);
  return i;
}

}}
