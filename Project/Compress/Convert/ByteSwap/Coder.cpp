// Coder.cpp

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

STDMETHODIMP CByteSwap2::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  const kStep = 2;
  UINT32 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));
    if (aProcessedSize == 0)
      return  anOutStream->Write(m_Buffer, aBufferPos, NULL);

    UINT32 anEndPos = aBufferPos + aProcessedSize;
    for (UINT32 aCurPos = 0; aCurPos + kStep <= anEndPos; aCurPos += kStep)
    {
      BYTE aData[kStep];
      for (int i = 0; i < kStep; i++)
        aData[i] = m_Buffer[aCurPos + i];
      for (i = 0; i < kStep; i++)
        m_Buffer[aCurPos + i] = aData[kStep - 1 - i];
    }
    RETURN_IF_NOT_S_OK(anOutStream->Write(m_Buffer, aCurPos, &aProcessedSize));
    if (aCurPos != aProcessedSize)
      return E_FAIL;
    aBufferPos = 0;
    while(aCurPos < anEndPos)
      m_Buffer[aBufferPos++] = m_Buffer[aCurPos++];
  }
}


STDMETHODIMP CByteSwap4::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  const kStep = 4;
  UINT32 aNowPos = 0;
  UINT32 aBufferPos = 0;
  UINT32 aProcessedSize;
  while(true)
  {
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));
    if (aProcessedSize == 0)
      return  anOutStream->Write(m_Buffer, aBufferPos, NULL);

    UINT32 anEndPos = aBufferPos + aProcessedSize;
    for (UINT32 aCurPos = 0; aCurPos + kStep <= anEndPos; aCurPos += kStep)
    {
      BYTE aData[kStep];
      for (int i = 0; i < kStep; i++)
        aData[i] = m_Buffer[aCurPos + i];
      for (i = 0; i < kStep; i++)
        m_Buffer[aCurPos + i] = aData[kStep - 1 - i];
    }
    RETURN_IF_NOT_S_OK(anOutStream->Write(m_Buffer, aCurPos, &aProcessedSize));
    if (aCurPos != aProcessedSize)
      return E_FAIL;
    aBufferPos = 0;
    while(aCurPos < anEndPos)
      m_Buffer[aBufferPos++] = m_Buffer[aCurPos++];
  }
}

