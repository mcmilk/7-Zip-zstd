// BranchX86.c

#include "BranchX86.h"

static int inline Test86MSByte(Byte b)
{
  return (b == 0 || b == 0xFF);
}

const int kMaskToAllowedStatus[8] = {1, 1, 1, 0, 1, 0, 0, 0};
const Byte kMaskToBitNumber[8] = {0, 1, 2, 2, 3, 3, 3, 3};

void x86_Convert_Init(UInt32 *prevMask, UInt32 *prevPos)
{
  *prevMask = 0;
  *prevPos = (-5);
}

UInt32 x86_Convert(Byte *buffer, UInt32 endPos, UInt32 nowPos, 
    UInt32 *prevMask, UInt32 *prevPos, int encoding)
{
  UInt32 bufferPos = 0;

  if (endPos < 5)
    return 0;
  
  if (nowPos - *prevPos > 5)
    *prevPos = nowPos - 5;
  
  UInt32 limit = endPos - 5;
  while(bufferPos <= limit)
  {
    Byte b = buffer[bufferPos];
    if (b != 0xE8 && b != 0xE9)
    {
      bufferPos++;
      continue;
    }
    UInt32 offset = (nowPos + bufferPos - *prevPos);
    *prevPos = (nowPos + bufferPos);
    if (offset > 5)
      *prevMask = 0;
    else
    {
      for (UInt32 i = 0; i < offset; i++)
      {
        *prevMask &= 0x77;
        *prevMask <<= 1;
      }
    }
    Byte &nextByte = buffer[bufferPos + 4];
    if (Test86MSByte(nextByte) && kMaskToAllowedStatus[(*prevMask >> 1) & 0x7] && 
      (*prevMask >> 1) < 0x10)
    {
      UInt32 src = 
        (UInt32(nextByte) << 24) |
        (UInt32(buffer[bufferPos + 3]) << 16) |
        (UInt32(buffer[bufferPos + 2]) << 8) |
        (buffer[bufferPos + 1]);
      
      UInt32 dest;
      while(true)
      {
        if (encoding)
          dest = (nowPos + bufferPos + 5) + src;
        else
          dest = src - (nowPos + bufferPos + 5);
        if (*prevMask == 0)
          break;
        UInt32 index = kMaskToBitNumber[*prevMask >> 1];
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
      *prevMask = 0;
    }
    else
    {
      bufferPos++;
      *prevMask |= 1;
      if (Test86MSByte(nextByte))
        *prevMask |= 0x10;
    }
  }
  return bufferPos;
}
