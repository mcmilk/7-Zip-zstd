// Mtf8.h

#ifndef __MTF8_H
#define __MTF8_H

#include "Common/Types.h"

namespace NCompress {

class CMtf8Encoder
{
public:
  Byte Buffer[256];
  int FindAndMove(Byte v)
  {
    int pos;
    for (pos = 0; Buffer[pos] != v; pos++);
    int resPos = pos;
    for (; pos >= 8; pos -= 8)
    {
      Buffer[pos] = Buffer[pos - 1];
      Buffer[pos - 1] = Buffer[pos - 2];
      Buffer[pos - 2] = Buffer[pos - 3];
      Buffer[pos - 3] = Buffer[pos - 4];
      Buffer[pos - 4] = Buffer[pos - 5];
      Buffer[pos - 5] = Buffer[pos - 6];
      Buffer[pos - 6] = Buffer[pos - 7];
      Buffer[pos - 7] = Buffer[pos - 8];
    }
    for (; pos > 0; pos--)
      Buffer[pos] = Buffer[pos - 1];
    Buffer[0] = v;
    return resPos;
  }
};

class CMtf8Decoder
{
public:
  Byte Buffer[256];
  void Init(int /* size */) {};
  Byte GetHead() const { return Buffer[0]; }
  Byte GetAndMove(int pos)
  {
    Byte res = Buffer[pos];
    for (; pos >= 8; pos -= 8)
    {
      Buffer[pos] = Buffer[pos - 1];
      Buffer[pos - 1] = Buffer[pos - 2];
      Buffer[pos - 2] = Buffer[pos - 3];
      Buffer[pos - 3] = Buffer[pos - 4];
      Buffer[pos - 4] = Buffer[pos - 5];
      Buffer[pos - 5] = Buffer[pos - 6];
      Buffer[pos - 6] = Buffer[pos - 7];
      Buffer[pos - 7] = Buffer[pos - 8];
    }
    for (; pos > 0; pos--)
      Buffer[pos] = Buffer[pos - 1];
    Buffer[0] = res;
    return res;
  }
};

/*
const int kSmallSize = 64;
class CMtf8Decoder
{
  Byte SmallBuffer[kSmallSize];
  int SmallSize;
  Byte Counts[16];
  int Size;
public:
  Byte Buffer[256];

  Byte GetHead() const 
  { 
    if (SmallSize > 0)
      return SmallBuffer[kSmallSize - SmallSize];
    return Buffer[0];
  }

  void Init(int size) 
  { 
    Size = size; 
    SmallSize = 0; 
    for (int i = 0; i < 16; i++)
    {
      Counts[i] = ((size >= 16) ? 16 : size);
      size -= Counts[i];
    }
  }

  Byte GetAndMove(int pos)
  {
    if (pos < SmallSize)
    {
      Byte *p = SmallBuffer + kSmallSize - SmallSize;
      Byte res = p[pos];
      for (; pos > 0; pos--)
        p[pos] = p[pos - 1];
      SmallBuffer[kSmallSize - SmallSize] = res;
      return res;
    }
    if (SmallSize == kSmallSize)
    {
      int i = Size - 1;
      int g = 16;
      do
      {
        g--;
        int offset = (g << 4);
        for (int t = Counts[g] - 1; t >= 0; t--, i--)
          Buffer[i] = Buffer[offset + t];
      }
      while(g != 0);
      
      for (i = kSmallSize - 1; i >= 0; i--)
        Buffer[i] = SmallBuffer[i];
      Init(Size);
    }
    pos -= SmallSize;
    int g;
    for (g = 0; pos >= Counts[g]; g++)
      pos -= Counts[g];
    int offset = (g << 4);
    Byte res = Buffer[offset + pos];
    for (pos; pos < 16 - 1; pos++)
      Buffer[offset + pos] = Buffer[offset + pos + 1];
    
    SmallSize++;
    SmallBuffer[kSmallSize - SmallSize] = res;

    Counts[g]--;
    return res;
  }
};
*/

}
#endif
