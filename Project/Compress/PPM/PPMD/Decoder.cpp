// Compress/Associative/Decoder.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Windows/Defs.h"

#include "Decoder.h"

using namespace NCompression;
using namespace NArithmetic;

namespace NCompress {
namespace NPPMD {


STDMETHODIMP CDecoder::SetDecoderProperties(ISequentialInStream *anInStream)
{
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(anInStream->Read(&m_Order, 
      sizeof(m_Order), &aProcessedSize));
  if (aProcessedSize != sizeof(m_Order))
    return E_FAIL;
  RETURN_IF_NOT_S_OK(anInStream->Read(&m_UsedMemorySize, 
      sizeof(m_UsedMemorySize), &aProcessedSize));
  if (aProcessedSize != sizeof(m_UsedMemorySize))
    return E_FAIL;
  return S_OK;
}

class CDecoderFlusher
{
  CDecoder *m_Coder;
public:
  CDecoderFlusher(CDecoder *aCoder): m_Coder(aCoder) {}
  ~CDecoderFlusher()
  {
    m_Coder->Flush();
    m_Coder->ReleaseStreams();
  }
};

UINT32 GetMatchLen(const BYTE *aPointer1, const BYTE *aPointer2, 
    UINT32 aLimit)
{  
  for(UINT32 i = 0; i < aLimit && *aPointer1 == *aPointer2; 
      aPointer1++, aPointer2++, i++);
  return i;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  m_RangeDecoder.Init(anInStream);
  m_OutStream.Init(anOutStream);

  CDecoderFlusher aFlusher(this);

  if (anOutSize == NULL)
    return E_INVALIDARG;

  UINT64 aProgressPosValuePrev = 0, aPos = 0;

  try
  {
    if ( !m_Info.m_SubAllocator.StartSubAllocator(m_UsedMemorySize) ) 
      return E_OUTOFMEMORY;
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }

  // m_Info.Init();
  // m_Info.MaxOrder = m_Order; 
  m_Info.MaxOrder = 0;
  m_Info.StartModelRare(m_Order);
  
  while(aPos < *anOutSize)
  {
    aPos++;
    m_OutStream.WriteByte(m_Info.DecodeSymbol(&m_RangeDecoder));
    if (aPos - aProgressPosValuePrev >= (1 << 18) && aProgress != NULL)
    {
      UINT64 anInSize = m_RangeDecoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&anInSize, &aPos));
      aProgressPosValuePrev = aPos;
    }
  }
  return S_OK;
}


}}
