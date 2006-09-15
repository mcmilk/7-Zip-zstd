// Crypto/ZipCipher.h

#include "StdAfx.h"

#include "ZipCipher.h"
#include "Windows/Defs.h"

#include "../../Common/StreamUtils.h"
#include "../Hash/RandGen.h"

namespace NCrypto {
namespace NZip {

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

  UInt32 processedSize;
  _cipher.EncryptHeader(header);
  RINOK(WriteStream(outStream, header, kHeaderSize, &processedSize));
  if (processedSize != kHeaderSize)
    return E_FAIL;
  return S_OK;
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
  UInt32 processedSize;
  RINOK(ReadStream(inStream, header, kHeaderSize, &processedSize));
  if (processedSize != kHeaderSize)
    return E_FAIL;
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
