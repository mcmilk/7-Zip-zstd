// IA64.cpp

#include "StdAfx.h"
#include "IA64.h"

#include "Windows/Defs.h"

const BYTE kBranchTable[32] = 
{ 
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  4, 4, 6, 6, 0, 0, 7, 7,
  4, 4, 0, 0, 4, 4, 0, 0 
};


static HRESULT BC_IA64_Code(ISequentialInStream *inStream,
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
    if (endPos < 16)
    {
      if (endPos > 0)
      {
        RETURN_IF_NOT_S_OK(outStream->Write(buffer, endPos, &processedSize));
        if (endPos != processedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    for (bufferPos = 0; bufferPos <= endPos - 16; bufferPos += 16)
    {
      UINT32 instrTemplate = buffer[bufferPos] & 0x1F;
      // ofs << hex << setw(4) << instrTemplate << endl;
      UINT32 mask = kBranchTable[instrTemplate];
      UINT32 bitPos = 5;
      for (int slot = 0; slot < 3; slot++, bitPos += 41)
      {
        if (((mask >> slot) & 1) == 0)
          continue;
        UINT32 bytePos = (bitPos >> 3);
        UINT32 bitRes = bitPos & 0x7;
        UINT64 instruction = *(UINT64 *)(buffer + bufferPos + bytePos);
        UINT64 instNorm = instruction >> bitRes;
        if (((instNorm >> 37) & 0xF) == 0x5 
            &&  ((instNorm >> 9) & 0x7) == 0 
            // &&  (instNorm & 0x3F)== 0 
            )
        {
          UINT32 src = UINT32((instNorm >> 13) & 0xFFFFF);
          src |= ((instNorm >> 36) & 1) << 20;
  
          src <<= 4;

          UINT32 dest;
          if (encoding)
            dest = nowPos + bufferPos + src;
          else
            dest = src - (nowPos + bufferPos);

          dest >>= 4;

          UINT64  instNorm2 = instNorm;

          instNorm &= ~(UINT64(0x8FFFFF) << 13);
          instNorm |= (UINT64(dest & 0xFFFFF) << 13);
          instNorm |= (UINT64(dest & 0x100000) << (36 - 20));

          instruction &= (1 << bitRes) - 1;
          instruction |= (instNorm << bitRes);
          *(UINT64 *)(buffer + bufferPos + bytePos) = instruction;
        }
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

MyClassImp(BC_IA64)


