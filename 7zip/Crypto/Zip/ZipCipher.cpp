// Crypto/ZipCipher.h

#include "StdAfx.h"

#include "ZipCipher.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NZip {

/*
const int kBufferSize = 1 << 17;

CBuffer2::CBuffer2():
  _buffer(0)
{
  _buffer = new Byte[kBufferSize];
}

CBuffer2::~CBuffer2()
{
  delete []_buffer;
}
*/

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
  CRandom random;
  random.Init(::GetTickCount());

  UInt64 nowPos = 0;
  Byte header[kHeaderSize];
  for (int i = 0; i < kHeaderSize - 2; i++)
  {
    header[i] = Byte(random.Generate());
  }
  header[kHeaderSize - 1] = Byte(_crc >> 24);
  header[kHeaderSize - 2] = Byte(_crc >> 16);

  UInt32 processedSize;
  _cipher.EncryptHeader(header);
  RINOK(outStream->Write(header, kHeaderSize, &processedSize));
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
  UInt64 nowPos = 0;
  Byte header[kHeaderSize];
  UInt32 processedSize;
  RINOK(inStream->Read(header, kHeaderSize, &processedSize));
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
