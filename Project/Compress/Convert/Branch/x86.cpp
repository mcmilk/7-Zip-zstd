// x86.h

#include "StdAfx.h"
#include "x86.h"

#include "Windows/Defs.h"

static bool inline Test86MSByte(BYTE aByte)
{
  return (aByte == 0 || aByte == 0xFF);
}

const bool kMaskToAllowedStatus[8] = {true, true, true, false, true, false, false, false};
const BYTE kMaskToBitNumber[8] = {0, 1, 2, 2, 3, 3, 3, 3};

static HRESULT BCJ_x86_Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress, BYTE *aBuffer, bool anEncoding)
{
  UINT64 aNowPos64 = 0;
  UINT32 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  UINT32 aPrevMask = 0;
  UINT32 aPrevPos = (- 5);

  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(aBuffer + aBufferPos, aSize, &aProcessedSize));
    UINT32 anEndPos = aBufferPos + aProcessedSize;
    if (anEndPos < 5)
    {
      if (anEndPos > 0)
      {
        RETURN_IF_NOT_S_OK(anOutStream->Write(aBuffer, anEndPos, &aProcessedSize));
        if (anEndPos != aProcessedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    aBufferPos = 0;
    if (aNowPos - aPrevPos > 5)
      aPrevPos = aNowPos - 5;

    UINT32 aLimit = anEndPos - 5;
    while(aBufferPos <= aLimit)
    {
      if (aBuffer[aBufferPos] != 0xE8 && aBuffer[aBufferPos] != 0xE9)
      {
        aBufferPos++;
        continue;
      }
      UINT32 anOffset = (aNowPos + aBufferPos - aPrevPos);
      aPrevPos = (aNowPos + aBufferPos);
      if (anOffset > 5)
        aPrevMask = 0;
      else
      {
        for (UINT32 i = 0; i < anOffset; i++)
        {
          aPrevMask &= 0x77;
          aPrevMask <<= 1;
        }
      }
      BYTE &aNextByte = aBuffer[aBufferPos + 4];
      if (Test86MSByte(aNextByte) && kMaskToAllowedStatus[(aPrevMask >> 1) & 0x7] && 
        (aPrevMask >> 1) < 0x10)
      {
        UINT32 aSrc = 
          (UINT32(aNextByte) << 24) |
          (UINT32(aBuffer[aBufferPos + 3]) << 16) |
          (UINT32(aBuffer[aBufferPos + 2]) << 8) |
          (aBuffer[aBufferPos + 1]);

        UINT32 aDest;
        while(true)
        {
          if (anEncoding)
            aDest = (aNowPos + aBufferPos + 5) + aSrc;
          else
            aDest = aSrc - (aNowPos + aBufferPos + 5);
          if (aPrevMask == 0)
            break;
          UINT32 anIndex = kMaskToBitNumber[aPrevMask >> 1];
          if (!Test86MSByte(aDest >> (24 - anIndex * 8)))
            break;
          aSrc = aDest ^ ((1 << (32 - anIndex * 8)) - 1);
          // aSrc = aDest;
        }
        aNextByte = ~(((aDest >> 24) & 1) - 1);
        aBuffer[aBufferPos + 3] = (aDest >> 16);
        aBuffer[aBufferPos + 2] = (aDest >> 8);
        aBuffer[aBufferPos + 1] = aDest;
        aBufferPos += 5;
        aPrevMask = 0;
      }
      else
      {
        aBufferPos++;
        aPrevMask |= 1;
        if (Test86MSByte(aNextByte))
          aPrevMask |= 0x10;
      }
    }
    aNowPos += aBufferPos;
    aNowPos64 += aBufferPos;
    RETURN_IF_NOT_S_OK(anOutStream->Write(aBuffer, aBufferPos, &aProcessedSize));
    if (aBufferPos != aProcessedSize)
      return E_FAIL;
    if (aProgress != NULL)
    {
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aNowPos64, &aNowPos64));
    }
    
    UINT32 i = 0;
    while(aBufferPos < anEndPos)
      aBuffer[i++] = aBuffer[aBufferPos++];
    aBufferPos = i;
  }
}


MyClassImp(BCJ_x86)
