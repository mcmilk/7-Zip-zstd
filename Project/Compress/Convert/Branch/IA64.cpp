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


static HRESULT BC_IA64_Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress, BYTE *aBuffer, bool anEncoding)
{
  UINT32 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(aBuffer + aBufferPos, aSize, &aProcessedSize));
    UINT32 anEndPos = aBufferPos + aProcessedSize;
    if (anEndPos < 16)
    {
      if (anEndPos > 0)
      {
        RETURN_IF_NOT_S_OK(anOutStream->Write(aBuffer, anEndPos, &aProcessedSize));
        if (anEndPos != aProcessedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    for (aBufferPos = 0; aBufferPos <= anEndPos - 16; aBufferPos += 16)
    {
      UINT32 aTemplate = aBuffer[aBufferPos] & 0x1F;
      // ofs << hex << setw(4) << aTemplate << endl;
      UINT32 aMask = kBranchTable[aTemplate];
      UINT32 aBitPos = 5;
      for (int aSlot = 0; aSlot < 3; aSlot++, aBitPos += 41)
      {
        if (((aMask >> aSlot) & 1) == 0)
          continue;
        UINT32 aBytePos = (aBitPos >> 3);
        UINT32 aBitRes = aBitPos & 0x7;
        UINT64 aInstruction = *(UINT64 *)(aBuffer + aBufferPos + aBytePos);
        UINT64 aInstNorm = aInstruction >> aBitRes;
        if (((aInstNorm >> 37) & 0xF) == 0x5 
            &&  ((aInstNorm >> 9) & 0x7) == 0 
            // &&  (aInstNorm & 0x3F)== 0 
            )
        {
          UINT32 aSrc = UINT32((aInstNorm >> 13) & 0xFFFFF);
          aSrc |= ((aInstNorm >> 36) & 1) << 20;
  
          aSrc <<= 4;

          UINT32 aDest;
          if (anEncoding)
            aDest = aNowPos + aBufferPos + aSrc;
          else
            aDest = aSrc - (aNowPos + aBufferPos);

          aDest >>= 4;

          UINT64  aInstNorm2 = aInstNorm;

          aInstNorm &= ~(UINT64(0x8FFFFF) << 13);
          aInstNorm |= (UINT64(aDest & 0xFFFFF) << 13);
          aInstNorm |= (UINT64(aDest & 0x100000) << (36 - 20));

          aInstruction &= (1 << aBitRes) - 1;
          aInstruction |= (aInstNorm << aBitRes);
          *(UINT64 *)(aBuffer + aBufferPos + aBytePos) = aInstruction;
        }
      }
    }
    aNowPos += aBufferPos;
    RETURN_IF_NOT_S_OK(anOutStream->Write(aBuffer, aBufferPos, &aProcessedSize));
    if (aBufferPos != aProcessedSize)
      return E_FAIL;
    
    UINT32 i = 0;
    while(aBufferPos < anEndPos)
      aBuffer[i++] = aBuffer[aBufferPos++];
    aBufferPos = i;
  }
}

MyClassImp(BC_IA64)


