// Compress/Associative/Decoder.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Windows/Defs.h"

#include "Decoder.h"

using namespace NCompression;
using namespace NArithmetic;

namespace NCompress {
namespace NPPMD {


STDMETHODIMP CDecoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  UINT32 processedSize;
  RETURN_IF_NOT_S_OK(inStream->Read(&_order, 
      sizeof(_order), &processedSize));
  if (processedSize != sizeof(_order))
    return E_FAIL;
  RETURN_IF_NOT_S_OK(inStream->Read(&_usedMemorySize, 
      sizeof(_usedMemorySize), &processedSize));
  if (processedSize != sizeof(_usedMemorySize))
    return E_FAIL;
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

UINT32 GetMatchLen(const BYTE *pointer1, const BYTE *pointer2, 
    UINT32 limit)
{  
  UINT32 i;
  for(i = 0; i < limit && *pointer1 == *pointer2; 
      pointer1++, pointer2++, i++);
  return i;
}

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress)
{
  _rangeDecoder.Init(inStream);
  _outStream.Init(outStream);

  CDecoderFlusher flusher(this);

  /*
  if (outSize == NULL)
    return E_INVALIDARG;
  */

  UINT64 progressPosValuePrev = 0, pos = 0;

  try
  {
    if (!_info.SubAllocator.StartSubAllocator(_usedMemorySize)) 
      return E_OUTOFMEMORY;
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }

  // _info.Init();
  // _info.MaxOrder = _order; 
  _info.MaxOrder = 0;
  _info.StartModelRare(_order);

  UINT64 size = (outSize == NULL) ? (UINT64)(INT64)(-1) : *outSize;

  while(pos < size)
  {
    pos++;
    int symbol = _info.DecodeSymbol(&_rangeDecoder);
    if (symbol < 0)
      return S_OK;
    _outStream.WriteByte(symbol);
    if (pos - progressPosValuePrev >= (1 << 18) && progress != NULL)
    {
      UINT64 inSize = _rangeDecoder.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&inSize, &pos));
      progressPosValuePrev = pos;
    }
  }
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}


}}
