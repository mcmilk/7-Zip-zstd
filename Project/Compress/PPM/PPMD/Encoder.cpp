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
  _rangeEncoder = aRangeEncoder;
  RETURN_IF_NOT_S_OK(_rangeEncoder.QueryInterface(&m_InitOutCoder));

  return S_OK;
}
*/

STDMETHODIMP CEncoder::SetCoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
{
  for (UINT32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &aProperty = properties[i];
    switch(propIDs[i])
    {
      case NEncodedStreamProperies::kUsedMemorySize:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        if (aProperty.ulVal < kMinMemSize)
          return E_INVALIDARG;
        _usedMemorySize = aProperty.ulVal;
        break;
      case NEncodedStreamProperies::kOrder:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        if (aProperty.ulVal < kMinOrder || aProperty.ulVal > kMaxOrderCompress)
          return E_INVALIDARG;
        _order = BYTE(aProperty.ulVal);
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  RETURN_IF_NOT_S_OK(outStream->Write(&_order, sizeof(_order), NULL));
  return outStream->Write(&_usedMemorySize, sizeof(_usedMemorySize), NULL);
}

const kUsedMemorySizeDefault = (1 << 24);
const kOrderDefault = 6;

CEncoder::CEncoder():
  _usedMemorySize(kUsedMemorySizeDefault),
  _order(kOrderDefault)
{
  // SubAllocator.StartSubAllocator(kSubAllocator);
}


HRESULT CEncoder::Flush()
{
  _rangeEncoder.FlushData();
  return _rangeEncoder.FlushStream();
}

class CEncoderFlusher
{
  CEncoder *_encoder;
public:
  CEncoderFlusher(CEncoder *encoder): _encoder(encoder) {}
  ~CEncoderFlusher()
  {
    _encoder->Flush();
    _encoder->ReleaseStreams();
  }
};



HRESULT CEncoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  _inStream.Init(inStream);
  _rangeEncoder.Init(outStream);

  CEncoderFlusher aFlusher(this);

  UINT64 pos = 0;
  UINT64 prevProgressPos = 0;

  try
  {
    if ( !_info.SubAllocator.StartSubAllocator(_usedMemorySize) ) 
      return E_OUTOFMEMORY;
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }


  _info.MaxOrder = 0;
  _info.StartModelRare(_order);


  while (true)
  {
    BYTE symbol;
    if (!_inStream.ReadByte(symbol))
    {
      // here we can write End Mark for stream version. 
      // In current version this feature is not used.
      // _info.EncodeSymbol(-1, &_rangeEncoder);   

      return S_OK;
    }
    _info.EncodeSymbol(symbol, &_rangeEncoder);   
    pos++;
    if (pos - prevProgressPos >= (1 << 18) && progress != NULL)
    {
      UINT64 outSize = _rangeEncoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&pos, &outSize));
      prevProgressPos = pos;
    }
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}

}}

