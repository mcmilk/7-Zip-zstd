// Crypto/Rar20/Encoder.h

#include "StdAfx.h"

#include "Coder.h"
#include "Windows/Defs.h"


namespace NCrypto {
namespace NZip {

const kBufferSize = 1 << 17;

CBuffer2::CBuffer2():
  m_Buffer(0)
{
  m_Buffer = new BYTE[kBufferSize];
}

CBuffer2::~CBuffer2()
{
  delete []m_Buffer;
}


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

  BYTE aHeader[kHeaderSize];
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(anInStream->Read(aHeader, kHeaderSize, &aProcessedSize));
  if (aProcessedSize != kHeaderSize)
    return E_FAIL;
  m_Data.DecryptHeader(aHeader);

  while(true)
  {
    UINT32 aBufferPos = 0;
    UINT32 aSize = kBufferSize - aBufferPos;
    RETURN_IF_NOT_S_OK(anInStream->Read(m_Buffer + aBufferPos, aSize, &aProcessedSize));

    UINT32 anEndPos = aBufferPos + aProcessedSize;
    for (;aBufferPos < anEndPos; aBufferPos++)
      m_Buffer[aBufferPos] = m_Data.DecryptByte(m_Buffer[aBufferPos]);

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
  }
}

}}
