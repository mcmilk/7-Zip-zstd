// Compress/Associative/Encoder.h

#include "StdAfx.h"

#include "Windows/Defs.h"

// #include <fstream.h>
// #include <iomanip.h>

#include "Common/Defs.h"

#include "Encoder.h"

using namespace NCompression;
using namespace NArithmetic;

namespace NCompress {
namespace NPPMD {

/*
UINT32 g_NumInner = 0;
UINT32 g_InnerCycles = 0;

UINT32 g_Encode2 = 0;
UINT32 g_Encode2Cycles = 0;
UINT32 g_Encode2Cycles2 = 0;

class CCounter
{
public:
  CCounter() {}
  ~CCounter() 
  {
    ofstream ofs("Res.dat");
    ofs << "innerEncode1    = " << setw(10) << g_NumInner << endl;
    ofs << "g_InnerCycles   = " << setw(10) << g_InnerCycles << endl;
    ofs << "g_Encode2       = " << setw(10) << g_Encode2 << endl;
    ofs << "g_Encode2Cycles = " << setw(10) << g_Encode2Cycles << endl;
    ofs << "g_Encode2Cycles2= " << setw(10) << g_Encode2Cycles2 << endl;
    
  }
};

CCounter g_Counter;


/*
// ISetRangeEncoder
STDMETHODIMP CEncoder::SetRangeEncoder(CRangeEncoder *aRangeEncoder)
{
  m_RangeEncoder = aRangeEncoder;
  RETURN_IF_NOT_S_OK(m_RangeEncoder.QueryInterface(&m_InitOutCoder));

  return S_OK;
}
*/

STDMETHODIMP CEncoder::SetCoderProperties2(const PROPID *aPropIDs, 
    const PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  for (UINT32 i = 0; i < aNumProperties; i++)
  {
    const PROPVARIANT &aProperty = aProperties[i];
    switch(aPropIDs[i])
    {
      case NEncodedStreamProperies::kUsedMemorySize:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        if (aProperty.ulVal < kMinMemSize)
          return E_INVALIDARG;
        m_UsedMemorySize = aProperty.ulVal;
        break;
      case NEncodedStreamProperies::kOrder:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        if (aProperty.ulVal < kMinOrder || aProperty.ulVal > kMaxOrderCompress)
          return E_INVALIDARG;
        m_Order = BYTE(aProperty.ulVal);
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *anOutStreams)
{ 
  RETURN_IF_NOT_S_OK(anOutStreams->Write(&m_Order, sizeof(m_Order), NULL));
  return anOutStreams->Write(&m_UsedMemorySize, sizeof(m_UsedMemorySize), NULL);
}

const kUsedMemorySizeDefault = (1 << 24);
const kOrderDefault = 6;

CEncoder::CEncoder():
  m_UsedMemorySize(kUsedMemorySizeDefault),
  m_Order(kOrderDefault)
{
  // m_SubAllocator.StartSubAllocator(kSubAllocator);
}


HRESULT CEncoder::Flush()
{
  m_RangeEncoder.FlushData();
  return m_RangeEncoder.FlushStream();
}

class CEncoderFlusher
{
  CEncoder *m_Encoder;
public:
  CEncoderFlusher(CEncoder *anEncoder): m_Encoder(anEncoder) {}
  ~CEncoderFlusher()
  {
    m_Encoder->Flush();
    m_Encoder->ReleaseStreams();
  }
};



HRESULT CEncoder::CodeReal(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, 
      const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress)
{
  m_InStream.Init(anInStream);
  m_RangeEncoder.Init(anOutStream);

  CEncoderFlusher aFlusher(this);

  UINT64 aPos = 0;
  UINT64 aProgressPosValuePrev = 0;

  if ( !m_Info.m_SubAllocator.StartSubAllocator(m_UsedMemorySize) ) 
    return E_OUTOFMEMORY; // E_OUTOFMEMORY;

  m_Info.MaxOrder = 0;
  m_Info.StartModelRare(m_Order);


  while (true)
  {
    BYTE aByte;
    if (!m_InStream.ReadByte(aByte))
      return S_OK;
    m_Info.EncodeSymbol(aByte, &m_RangeEncoder);   
    aPos++;
    if (aPos - aProgressPosValuePrev >= (1 << 18) && aProgress != NULL)
    {
      UINT64 anOutSize = m_RangeEncoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aPos, &anOutSize));
      aProgressPosValuePrev = aPos;
    }
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(const NStream::COutByteWriteException &anException)
  {
    return anException.m_Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}

}}

