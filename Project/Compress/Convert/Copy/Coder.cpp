// Compression::CopyCoder.cpp

#include "StdAfx.h"

#include "Coder.h"
#include "Common/Defs.h"

namespace NCompress {

static const UINT32 kBufferSize = 1 << 21;

CCopyCoder::CCopyCoder()
{
  m_Buffer = new BYTE[kBufferSize];
}

CCopyCoder::~CCopyCoder()
{
  delete []m_Buffer;
}


STDMETHODIMP CCopyCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  UINT64 aTotalSize = 0;
  while(true)
  {
    UINT32 aProcessedSizeReal;
    HRESULT aResult;
    aResult = anInStream->Read(m_Buffer, kBufferSize, &aProcessedSizeReal);
    if (aResult != S_OK)
      return aResult;
    if(aProcessedSizeReal == 0)
      break;
    aResult = anOutStream->Write(m_Buffer, aProcessedSizeReal, NULL);
    if (aResult != S_OK)
      return aResult;
    aTotalSize += aProcessedSizeReal;
    if (aProgress != NULL)
    {
      HRESULT aResult = aProgress->SetRatioInfo(&aTotalSize, &aTotalSize);
      if (aResult != S_OK)
        return aResult;
    }
  }
  return S_OK;
}

}