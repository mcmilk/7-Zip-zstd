// ARM.cpp

#include "StdAfx.h"
#include "ARM.h"

#include "Windows/Defs.h"

static HRESULT BC_ARM_Code(ISequentialInStream *anInStream,
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
      if (aBuffer[aBufferPos + 3] == 0xeb)
      {
        UINT32 aSrc = 
            (aBuffer[aBufferPos + 2] << 16) |
            (aBuffer[aBufferPos + 1] << 8) |
            (aBuffer[aBufferPos + 0]);

        aSrc <<= 2;
        UINT32 aDest;
        if (anEncoding)
          aDest = aNowPos + aBufferPos + 8 + aSrc;
        else
          aDest = aSrc - (aNowPos + aBufferPos + 8);
        aDest >>= 2;
        aBuffer[aBufferPos + 2] = (aDest >> 16);
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

MyClassImp(BC_ARM)


