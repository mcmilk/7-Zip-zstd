// ARM.cpp

#include "StdAfx.h"
#include "ARM.h"

#include "Windows/Defs.h"

static HRESULT BC_ARM_Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress, BYTE *buffer, bool encoding)
{
  UINT32 nowPos = 0;
  UINT64 nowPos64 = 0;
  UINT32 bufferPos = 0;
  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RETURN_IF_NOT_S_OK(inStream->Read(buffer + bufferPos, size, &processedSize));
    UINT32 endPos = bufferPos + processedSize;
    if (endPos < 4)
    {
      if (endPos > 0)
      {
        RETURN_IF_NOT_S_OK(outStream->Write(buffer, endPos, &processedSize));
        if (endPos != processedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    for (bufferPos = 0; bufferPos <= endPos - 4; bufferPos += 4)
    {
      if (buffer[bufferPos + 3] == 0xeb)
      {
        UINT32 src = 
            (buffer[bufferPos + 2] << 16) |
            (buffer[bufferPos + 1] << 8) |
            (buffer[bufferPos + 0]);

        src <<= 2;
        UINT32 dest;
        if (encoding)
          dest = nowPos + bufferPos + 8 + src;
        else
          dest = src - (nowPos + bufferPos + 8);
        dest >>= 2;
        buffer[bufferPos + 2] = (dest >> 16);
        buffer[bufferPos + 1] = (dest >> 8);
        buffer[bufferPos + 0] = dest;
      }
    }
    nowPos += bufferPos;
    nowPos64 += bufferPos;
    RETURN_IF_NOT_S_OK(outStream->Write(buffer, bufferPos, &processedSize));
    if (bufferPos != processedSize)
      return E_FAIL;
    if (progress != NULL)
    {
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }

    UINT32 i = 0;
    while(bufferPos < endPos)
      buffer[i++] = buffer[bufferPos++];
    bufferPos = i;
  }
}

MyClassImp(BC_ARM)
