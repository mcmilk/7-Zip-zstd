// Coder.cpp

#include "StdAfx.h"

#include "Coder.h"
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
  const kStep = 2;
  UINT32 nowPos = 0;
  UINT32 bufferPos = 0;
  UINT64 nowPos64 = 0;
  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RETURN_IF_NOT_S_OK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    if (processedSize == 0)
      return  outStream->Write(_buffer, bufferPos, NULL);

    UINT32 endPos = bufferPos + processedSize;
    for (UINT32 curPos = 0; curPos + kStep <= endPos; curPos += kStep)
    {
      BYTE data[kStep];
      for (int i = 0; i < kStep; i++)
        data[i] = _buffer[curPos + i];
      for (i = 0; i < kStep; i++)
        _buffer[curPos + i] = data[kStep - 1 - i];
    }
    RETURN_IF_NOT_S_OK(outStream->Write(_buffer, curPos, &processedSize));
    if (curPos != processedSize)
      return E_FAIL;
    nowPos64 += curPos;
    if (progress != NULL)
    {
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos64, &nowPos64));
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
  const kStep = 4;
  UINT32 nowPos = 0;
  UINT32 bufferPos = 0;
  UINT64 nowPos64 = 0;
  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RETURN_IF_NOT_S_OK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    if (processedSize == 0)
      return  outStream->Write(_buffer, bufferPos, NULL);

    UINT32 endPos = bufferPos + processedSize;
    for (UINT32 curPos = 0; curPos + kStep <= endPos; curPos += kStep)
    {
      BYTE data[kStep];
      for (int i = 0; i < kStep; i++)
        data[i] = _buffer[curPos + i];
      for (i = 0; i < kStep; i++)
        _buffer[curPos + i] = data[kStep - 1 - i];
    }
    RETURN_IF_NOT_S_OK(outStream->Write(_buffer, curPos, &processedSize));
    if (curPos != processedSize)
      return E_FAIL;
    nowPos64 += curPos;
    if (progress != NULL)
    {
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }
    bufferPos = 0;
    while(curPos < endPos)
      _buffer[bufferPos++] = _buffer[curPos++];
  }
}

