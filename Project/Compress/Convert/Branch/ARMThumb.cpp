// ARMThumb.cpp

#include "StdAfx.h"
#include "ARMThumb.h"

#include "Windows/Defs.h"

static HRESULT BC_ARMThumb_Code(ISequentialInStream *inStream,
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
    for (bufferPos = 0; bufferPos <= endPos - 4; bufferPos += 2)
    {
      if ((buffer[bufferPos + 1] & 0xF8) == 0xF0 && 
        (buffer[bufferPos + 3] & 0xF8) == 0xF8)
      {
        UINT32 src = 
            ((buffer[bufferPos + 1] & 0x7) << 19) |
            (buffer[bufferPos + 0] << 11) |
            ((buffer[bufferPos + 3] & 0x7) << 8) |
            (buffer[bufferPos + 2]);

        src <<= 1;
        UINT32 dest;
        if (encoding)
          dest = nowPos + bufferPos + 4 + src;
        else
          dest = src - (nowPos + bufferPos + 4);
        dest >>= 1;

        buffer[bufferPos + 1] = 0xF0 | ((dest >> 19) & 0x7);
        buffer[bufferPos + 0] = (dest >> 11);
        buffer[bufferPos + 3] = 0xF8 | ((dest >> 8) & 0x7);
        buffer[bufferPos + 2] = (dest);
        bufferPos += 2;
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

MyClassImp(BC_ARMThumb)


