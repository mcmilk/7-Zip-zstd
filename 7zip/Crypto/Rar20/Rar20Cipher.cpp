// Crypto/Rar20Cipher.cpp

#include "StdAfx.h"

#include "Rar20Cipher.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NRar20 {

static const int kBufferSize = 1 << 17;

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

/*
STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  UInt64 nowPos = 0;
  UInt32 bufferPos = 0;
  UInt32 processedSize;
  while(true)
  {
    UInt32 size = kBufferSize - bufferPos;
    RINOK(inStream->Read(_buffer + bufferPos, size, &processedSize));

    UInt32 anEndPos = bufferPos + processedSize;
    for (;bufferPos + 16 <= anEndPos; bufferPos += 16)
      _coder.DecryptBlock(_buffer + bufferPos);

    if (bufferPos == 0)
      return S_OK;

    if (outSize != NULL && nowPos + bufferPos > *outSize)
       bufferPos = UInt32(*outSize - nowPos);

    RINOK(outStream->Write(_buffer, bufferPos, &processedSize));
    if (bufferPos != processedSize)
      return E_FAIL;

    nowPos +=  processedSize;
    if (outSize != NULL && nowPos == *outSize)
      return S_OK;

    int i = 0;
    while(bufferPos < anEndPos)
      _buffer[i++] = _buffer[bufferPos++];
    bufferPos = i;
  }
}
*/

}}
