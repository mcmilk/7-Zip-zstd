// Crypto/Rar20/Encoder.h

#include "StdAfx.h"

#include "Coder.h"
#include "Windows/Defs.h"

const kBufferSize = 1 << 17;

CBuffer::CBuffer():
  m_Buffer(0)
{
  m_Buffer = new BYTE[kBufferSize];
}

CBuffer::~CBuffer()
{
  delete []m_Buffer;
}

namespace NCrypto {
namespace NRar20 {


/*
STDMETHODIMP CEncoder::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  UINT32 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));
    UINT32 anEndPos = aBufferPos + aProcessedSize;
    if (anEndPos < 5)
    {
      if (anEndPos > 0)
      {
        RETURN_IF_NOT_S_OK(anOutStream->Write(m_Buffer, anEndPos, &aProcessedSize));
        if (anEndPos != aProcessedSize)
          return E_FAIL;
      }
      return S_OK;
    }
    UINT32 aLimitPos = anEndPos - 4;
    
    aBufferPos = 0;
    while (aBufferPos < aLimitPos)
    {
      if (m_Buffer[aBufferPos] == 0xE8 || m_Buffer[aBufferPos] == 0xE9)
      {
        UINT32 anOffset = 0;
        UINT32 aLeft = 0;
        for(int j = 0; j < 4; j++, aLeft += 8)
          anOffset += UINT32(m_Buffer[aBufferPos + 1 + j]) << aLeft;
        UINT32 anAbsolute = ConvertCall(aNowPos + aBufferPos + 5, anOffset);
        for(j = 0; j < 4; j++, anAbsolute >>= 8)
          m_Buffer[aBufferPos + 1 + j] = (BYTE(anAbsolute & 0xFF));
        aBufferPos += 5;
      }
      else
        aBufferPos++;
    }
    aNowPos += aBufferPos;
    RETURN_IF_NOT_S_OK(anOutStream->Write(m_Buffer, aBufferPos, &aProcessedSize));
    if (aBufferPos != aProcessedSize)
      return E_FAIL;
    
    UINT32 i = 0;
    while(aBufferPos < anEndPos)
      m_Buffer[i++] = m_Buffer[aBufferPos++];
    aBufferPos = i;
  }
}
*/

STDMETHODIMP CDecoder::CryptoSetPassword(const BYTE *aData, UINT32 aSize)
{
  m_Data.SetPassword(aData, aSize);
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  UINT64 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));

    UINT32 anEndPos = aBufferPos + aProcessedSize;
    for (;aBufferPos + 16 <= anEndPos; aBufferPos += 16)
      m_Data.DecryptBlock(m_Buffer + aBufferPos);

    if (aBufferPos == 0)
      return S_OK;

    if (anOutSize != NULL && aNowPos + aBufferPos > *anOutSize)
       aBufferPos = UINT32(*anOutSize - aNowPos);

    RETURN_IF_NOT_S_OK(anOutStream->Write(m_Buffer, aBufferPos, &aProcessedSize));
    if (aBufferPos != aProcessedSize)
      return E_FAIL;

    aNowPos +=  aProcessedSize;
    if (anOutSize != NULL && aNowPos == *anOutSize)
      return S_OK;

    int i = 0;
    while(aBufferPos < anEndPos)
      m_Buffer[i++] = m_Buffer[aBufferPos++];
    aBufferPos = i;
  }
}

}}
