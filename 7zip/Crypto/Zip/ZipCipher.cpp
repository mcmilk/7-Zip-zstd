// Crypto/ZipCipher.h

#include "StdAfx.h"

#include "ZipCipher.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NZip {

const int kBufferSize = 1 << 17;

CBuffer2::CBuffer2():
  _buffer(0)
{
  _buffer = new BYTE[kBufferSize];
}

CBuffer2::~CBuffer2()
{
  delete []_buffer;
}

STDMETHODIMP CEncoder::CryptoSetPassword(const BYTE *data, UINT32 size)
{
  _cipher.SetPassword(data, size);
  return S_OK;
}

STDMETHODIMP CEncoder::CryptoSetCRC(UINT32 crc)
{
  _crc = crc;
  return S_OK;
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  CRandom random;
  random.Init(::GetTickCount());

  UINT64 nowPos = 0;
  BYTE header[kHeaderSize];
  for (int i = 0; i < kHeaderSize - 2; i++)
  {
    header[i] = BYTE(random.Generate());
  }
  header[kHeaderSize - 1] = BYTE(_crc >> 24);
  header[kHeaderSize - 2] = BYTE(_crc >> 16);

  UINT32 processedSize;
  _cipher.EncryptHeader(header);
  RINOK(outStream->Write(header, kHeaderSize, &processedSize));
  if (processedSize != kHeaderSize)
    return E_FAIL;

  while(true)
  {
    if (outSize != NULL && nowPos == *outSize)
      return S_OK;
    RINOK(inStream->Read(_buffer, kBufferSize, &processedSize));
    if (processedSize == 0)
      return S_OK;
    for (UINT32 i = 0; i < processedSize; i++)
      _buffer[i] = _cipher.EncryptByte(_buffer[i]);
    UINT32 size = processedSize;
    if (outSize != NULL && nowPos + size > *outSize)
       size = UINT32(*outSize - nowPos);
    RINOK(outStream->Write(_buffer, size, &processedSize));
    if (size != processedSize)
      return E_FAIL;
    nowPos +=  processedSize;
  }
}

STDMETHODIMP CDecoder::CryptoSetPassword(const BYTE *data, UINT32 size)
{
  _cipher.SetPassword(data, size);
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  UINT64 nowPos = 0;

  if (inSize != NULL && *inSize == 0)
    return S_OK;

  BYTE header[kHeaderSize];
  UINT32 processedSize;
  RINOK(inStream->Read(header, kHeaderSize, &processedSize));
  if (processedSize != kHeaderSize)
    return E_FAIL;
  _cipher.DecryptHeader(header);

  while(true)
  {
    if (outSize != NULL && nowPos == *outSize)
      return S_OK;
    RINOK(inStream->Read(_buffer, kBufferSize, &processedSize));
    if (processedSize == 0)
      return S_OK;
    for (UINT32 i = 0; i < processedSize; i++)
      _buffer[i] = _cipher.DecryptByte(_buffer[i]);
    UINT32 size = processedSize;
    if (outSize != NULL && nowPos + size > *outSize)
       size = UINT32(*outSize - nowPos);
    RINOK(outStream->Write(_buffer, size, &processedSize));
    if (size != processedSize)
      return E_FAIL;
    nowPos +=  processedSize;
  }
}

}}
