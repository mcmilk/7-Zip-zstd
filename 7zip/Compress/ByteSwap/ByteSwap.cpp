// Coder.cpp

#include "StdAfx.h"

#include "ByteSwap.h"
#include "Windows/Defs.h"

const kBufferSize = 1 << 17;

CBuffer::CBuffer():
  _buffer(0)
{
  _buffer = new BYTE[kBufferSize];
}

CBuffer::~CBuffer()
{
  delete []_buffer;
}

STDMETHODIMP CByteSwap2::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  const UINT32 kStep = 2;
  UINT32 bufferPos = 0;
  UINT64 nowPos64 = 0;
  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RINOK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    if (processedSize == 0)
      return outStream->Write(_buffer, bufferPos, NULL);

    UINT32 endPos = bufferPos + processedSize;
    for (UINT32 curPos = 0; curPos + kStep <= endPos; curPos += kStep)
    {
      BYTE data[kStep];
      data[0] = _buffer[curPos + 0];
      data[1] = _buffer[curPos + 1];
      _buffer[curPos + 0] = data[1];
      _buffer[curPos + 1] = data[0];
    }
    RINOK(outStream->Write(_buffer, curPos, &processedSize));
    if (curPos != processedSize)
      return E_FAIL;
    nowPos64 += curPos;
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }
    bufferPos = 0;
    while(curPos < endPos)
      _buffer[bufferPos++] = _buffer[curPos++];
  }
}

STDMETHODIMP CByteSwap4::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  const UINT32 kStep = 4;
  UINT32 bufferPos = 0;
  UINT64 nowPos64 = 0;
  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RINOK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    if (processedSize == 0)
      return outStream->Write(_buffer, bufferPos, NULL);

    UINT32 endPos = bufferPos + processedSize;
    for (UINT32 curPos = 0; curPos + kStep <= endPos; curPos += kStep)
    {
      BYTE data[kStep];
      data[0] = _buffer[curPos + 0];
      data[1] = _buffer[curPos + 1];
      data[2] = _buffer[curPos + 2];
      data[3] = _buffer[curPos + 3];
      _buffer[curPos + 0] = data[3];
      _buffer[curPos + 1] = data[2];
      _buffer[curPos + 2] = data[1];
      _buffer[curPos + 3] = data[0];
    }
    RINOK(outStream->Write(_buffer, curPos, &processedSize));
    if (curPos != processedSize)
      return E_FAIL;
    nowPos64 += curPos;
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }
    bufferPos = 0;
    while(curPos < endPos)
      _buffer[bufferPos++] = _buffer[curPos++];
  }
}

