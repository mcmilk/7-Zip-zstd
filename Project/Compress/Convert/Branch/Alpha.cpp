// Alpha.cpp

#include "StdAfx.h"
#include "Alpha.h"

#include "Windows/Defs.h"

static HRESULT BC_Alpha_Code(ISequentialInStream *anInStream,
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
      // if (aBuffer[aBufferPos + 3] == 0xc3 && (aBuffer[aBufferPos + 2] >> 5) == 7) // its jump
      // if (aBuffer[aBufferPos + 3] == 0xd3 && (aBuffer[aBufferPos + 2] >> 5) == 2)
      // if (aBuffer[aBufferPos + 3] == 0xd3)
      if ((aBuffer[aBufferPos + 3] >> 2) == 0x34)
      {
        UINT32 aSrc = 
            ((aBuffer[aBufferPos + 2] & 0x1F) << 16) |
            (aBuffer[aBufferPos + 1] << 8) |
            (aBuffer[aBufferPos + 0]);

        aSrc <<= 2;

        UINT32 aDest;
        if (anEncoding)
          aDest = (aNowPos + aBufferPos + 4) + aSrc;
        else
          aDest = aSrc - (aNowPos + aBufferPos + 4);
        aDest >>= 2;
        aDest &= 0x1FFFFF;
        aBuffer[aBufferPos + 2] &= (~0x1F);
        aBuffer[aBufferPos + 2] |= (aDest >> 16);
        aBuffer[aBufferPos + 1] = (aDest >> 8);
        aBuffer[aBufferPos + 0] = aDest;
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

MyClassImp(BC_Alpha)
