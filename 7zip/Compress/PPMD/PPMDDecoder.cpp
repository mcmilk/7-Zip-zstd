// PPMDDecoder.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Windows/Defs.h"

#include "PPMDDecoder.h"

namespace NCompress {
namespace NPPMD {

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *properties, UInt32 size)
{
  if (size < 5)
    return E_INVALIDARG;
  _order = properties[0];
  _usedMemorySize = 0;
  for (int i = 0; i < 4; i++)
    _usedMemorySize += ((UInt32)(properties[1 + i])) << (i * 8);
  return S_OK;
}

class CDecoderFlusher
{
  CDecoder *_coder;
public:
  CDecoderFlusher(CDecoder *coder): _coder(coder) {}
  ~CDecoderFlusher()
  {
    _coder->Flush();
    _coder->ReleaseStreams();
  }
};

UInt32 GetMatchLen(const Byte *pointer1, const Byte *pointer2, 
    UInt32 limit)
{  
  UInt32 i;
  for(i = 0; i < limit && *pointer1 == *pointer2; 
      pointer1++, pointer2++, i++);
  return i;
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  if (!_rangeDecoder.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!_outStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  _rangeDecoder.SetStream(inStream);
  _rangeDecoder.Init();
  _outStream.SetStream(outStream);
  _outStream.Init();

  CDecoderFlusher flusher(this);

  UInt64 progressPosValuePrev = 0, pos = 0;

  if (!_info.SubAllocator.StartSubAllocator(_usedMemorySize)) 
    return E_OUTOFMEMORY;

  // _info.Init();
  // _info.MaxOrder = _order; 
  _info.MaxOrder = 0;
  _info.StartModelRare(_order);

  UInt64 size = (outSize == NULL) ? (UInt64)(Int64)(-1) : *outSize;

  while(pos < size)
  {
    pos++;
    int symbol = _info.DecodeSymbol(&_rangeDecoder);
    if (symbol < 0)
      return S_OK;
    _outStream.WriteByte(symbol);
    if (pos - progressPosValuePrev >= (1 << 18) && progress != NULL)
    {
      UInt64 inSize = _rangeDecoder.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&inSize, &pos));
      progressPosValuePrev = pos;
    }
  }
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
}

}}
