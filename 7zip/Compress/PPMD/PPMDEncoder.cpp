// Compress/Associative/Encoder.h

#include "StdAfx.h"

#include "Windows/Defs.h"

// #include <fstream.h>
// #include <iomanip.h>

#include "Common/Defs.h"

#include "../../Common/StreamUtils.h"

#include "PPMDEncoder.h"

namespace NCompress {
namespace NPPMD {

const UInt32 kMinMemSize = (1 << 11); 
const UInt32 kMinOrder = 2;

/*
UInt32 g_NumInner = 0;
UInt32 g_InnerCycles = 0;

UInt32 g_Encode2 = 0;
UInt32 g_Encode2Cycles = 0;
UInt32 g_Encode2Cycles2 = 0;

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
*/

STDMETHODIMP CEncoder::SetCoderProperties(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
{
  for (UInt32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &prop = properties[i];
    switch(propIDs[i])
    {
      case NCoderPropID::kUsedMemorySize:
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        if (prop.ulVal < kMinMemSize || prop.ulVal > kMaxMemBlockSize)
          return E_INVALIDARG;
        _usedMemorySize = (UInt32)prop.ulVal;
        break;
      case NCoderPropID::kOrder:
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        if (prop.ulVal < kMinOrder || prop.ulVal > kMaxOrderCompress)
          return E_INVALIDARG;
        _order = (Byte)prop.ulVal;
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

STDMETHODIMP CEncoder::WriteCoderProperties(ISequentialOutStream *outStream)
{ 
  const UInt32 kPropSize = 5;
  Byte properties[kPropSize];
  properties[0] = _order;
  for (int i = 0; i < 4; i++)
    properties[1 + i] = Byte(_usedMemorySize >> (8 * i));
  return WriteStream(outStream, properties, kPropSize, NULL);
}

const UInt32 kUsedMemorySizeDefault = (1 << 24);
const int kOrderDefault = 6;

CEncoder::CEncoder():
  _usedMemorySize(kUsedMemorySizeDefault),
  _order(kOrderDefault)
{
}


HRESULT CEncoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 * /* inSize */, const UInt64 * /* outSize */,
      ICompressProgressInfo *progress)
{
  if (!_inStream.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!_rangeEncoder.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!_info.SubAllocator.StartSubAllocator(_usedMemorySize)) 
    return E_OUTOFMEMORY;

  _inStream.SetStream(inStream);
  _inStream.Init();

  _rangeEncoder.SetStream(outStream);
  _rangeEncoder.Init();

  CEncoderFlusher flusher(this);

  _info.MaxOrder = 0;
  _info.StartModelRare(_order);

  for (;;)
  {
    UInt32 size = (1 << 18);
    do
    {
      Byte symbol;
      if (!_inStream.ReadByte(symbol))
      {
        // here we can write End Mark for stream version. 
        // In current version this feature is not used.
        // _info.EncodeSymbol(-1, &_rangeEncoder);   
        return S_OK;
      }
      _info.EncodeSymbol(symbol, &_rangeEncoder);   
    }
    while (--size != 0);
    if (progress != NULL)
    {
      UInt64 inSize = _inStream.GetProcessedSize();
      UInt64 outSize = _rangeEncoder.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&inSize, &outSize));
    }
  }
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
}

}}
