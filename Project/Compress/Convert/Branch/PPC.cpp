// PPC.cpp

#include "StdAfx.h"
#include "PPC.h"

#include "Windows/Defs.h"

static HRESULT BC_PPC_B_Code(ISequentialInStream *anInStream,
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
    if (anEndPos < 4)
    {
      if (anEndPos > 0)
      {
        RETURN_IF_NOT_S_OK(anOutStream->Write(aBuffer, anEndPos, &aProcessedSize));
        if (anEndPos != aProcessedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    for (aBufferPos = 0; aBufferPos <= anEndPos - 4; aBufferPos += 4)
    {
      // PowerPC branch 6(48) 24(Offset) 1(Abs) 1(Link)
      if ((aBuffer[aBufferPos] >> 2) == 0x12 && 
            (
              (aBuffer[aBufferPos + 3] & 3) == 1 
              // || (aBuffer[aBufferPos+3] & 3) == 3
            )
          )
      {
        UINT32 aSrc = ((aBuffer[aBufferPos + 0] & 3) << 24) |
            (aBuffer[aBufferPos + 1] << 16) |
            (aBuffer[aBufferPos + 2] << 8) |
            (aBuffer[aBufferPos + 3] & (~3));

        UINT32 aDest;
        if (anEncoding)
          aDest = aNowPos + aBufferPos + aSrc;
        else
          aDest = aSrc - (aNowPos + aBufferPos);
        aBuffer[aBufferPos + 0] = 0x48 | ((aDest >> 24) &  0x3);
        aBuffer[aBufferPos + 1] = (aDest >> 16);
        aBuffer[aBufferPos + 2] = (aDest >> 8);
        aBuffer[aBufferPos + 3] &= 0x3;
        aBuffer[aBufferPos + 3] |= aDest;
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

MyClassImp(BC_PPC_B)

