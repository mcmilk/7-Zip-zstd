// x86.h

#include "StdAfx.h"
#include "x86.h"

#include "Windows/Defs.h"

static bool inline Test86MSByte(BYTE b)
{
  return (b == 0 || b == 0xFF);
}

const bool kMaskToAllowedStatus[8] = {true, true, true, false, true, false, false, false};
const BYTE kMaskToBitNumber[8] = {0, 1, 2, 2, 3, 3, 3, 3};

static HRESULT BCJ_x86_Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress, BYTE *buffer, bool encoding)
{
  UINT64 nowPos64 = 0;
  UINT32 nowPos = 0;
  UINT32 bufferPos = 0;
  UINT32 prevMask = 0;
  UINT32 prevPos = (- 5);

  while(true)
  {
    UINT32 processedSize;
    UINT32 size = kBufferSize - bufferPos;
    RINOK(inStream->Read(buffer + bufferPos, size, &processedSize));
    UINT32 endPos = bufferPos + processedSize;
    if (endPos < 5)
    {
      if (endPos > 0)
      {
        RINOK(outStream->Write(buffer, endPos, &processedSize));
        if (endPos != processedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    bufferPos = 0;
    if (nowPos - prevPos > 5)
      prevPos = nowPos - 5;

    UINT32 limit = endPos - 5;
    while(bufferPos <= limit)
    {
      if (buffer[bufferPos] != 0xE8 && buffer[bufferPos] != 0xE9)
      {
        bufferPos++;
        continue;
      }
      UINT32 offset = (nowPos + bufferPos - prevPos);
      prevPos = (nowPos + bufferPos);
      if (offset > 5)
        prevMask = 0;
      else
      {
        for (UINT32 i = 0; i < offset; i++)
        {
          prevMask &= 0x77;
          prevMask <<= 1;
        }
      }
      BYTE &nextByte = buffer[bufferPos + 4];
      if (Test86MSByte(nextByte) && kMaskToAllowedStatus[(prevMask >> 1) & 0x7] && 
        (prevMask >> 1) < 0x10)
      {
        UINT32 src = 
          (UINT32(nextByte) << 24) |
          (UINT32(buffer[bufferPos + 3]) << 16) |
          (UINT32(buffer[bufferPos + 2]) << 8) |
          (buffer[bufferPos + 1]);

        UINT32 dest;
        while(true)
        {
          if (encoding)
            dest = (nowPos + bufferPos + 5) + src;
          else
            dest = src - (nowPos + bufferPos + 5);
          if (prevMask == 0)
            break;
          UINT32 index = kMaskToBitNumber[prevMask >> 1];
          if (!Test86MSByte(dest >> (24 - index * 8)))
            break;
          src = dest ^ ((1 << (32 - index * 8)) - 1);
          // src = dest;
        }
        nextByte = ~(((dest >> 24) & 1) - 1);
        buffer[bufferPos + 3] = (dest >> 16);
        buffer[bufferPos + 2] = (dest >> 8);
        buffer[bufferPos + 1] = dest;
        bufferPos += 5;
        prevMask = 0;
      }
      else
      {
        bufferPos++;
        prevMask |= 1;
        if (Test86MSByte(nextByte))
          prevMask |= 0x10;
      }
    }
    nowPos += bufferPos;
    nowPos64 += bufferPos;
    RINOK(outStream->Write(buffer, bufferPos, &processedSize));
    if (bufferPos != processedSize)
      return E_FAIL;
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }
    
    UINT32 i = 0;
    while(bufferPos < endPos)
      buffer[i++] = buffer[bufferPos++];
    bufferPos = i;
  }
}


MyClassImp(BCJ_x86)
