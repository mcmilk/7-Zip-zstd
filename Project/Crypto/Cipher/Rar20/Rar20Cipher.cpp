// Crypto/Rar20Cipher.cpp

#include "StdAfx.h"

#include "Rar20Cipher.h"
#include "Windows/Defs.h"

namespace NCrypto {
namespace NRar20 {

static const int kBufferSize = 1 << 17;

CDecoder::CDecoder():
  _buffer(0)
{
  _buffer = new BYTE[kBufferSize];
}

CDecoder::~CDecoder()
{
  delete []_buffer;
}

STDMETHODIMP CDecoder::CryptoSetPassword(const BYTE *data, UINT32 size)
{
  _coder.SetPassword(data, size);
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  UINT64 nowPos = 0;
  UINT32 bufferPos = 0;
  UINT32 processedSize;
  while(true)
  {
    UINT32 size = kBufferSize - bufferPos;
    RETURN_IF_NOT_S_OK(inStream->Read(_buffer + bufferPos, size, &processedSize));

    UINT32 anEndPos = bufferPos + processedSize;
    for (;bufferPos + 16 <= anEndPos; bufferPos += 16)
      _coder.DecryptBlock(_buffer + bufferPos);

    if (bufferPos == 0)
      return S_OK;

    if (outSize != NULL && nowPos + bufferPos > *outSize)
       bufferPos = UINT32(*outSize - nowPos);

    RETURN_IF_NOT_S_OK(outStream->Write(_buffer, bufferPos, &processedSize));
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

}}
