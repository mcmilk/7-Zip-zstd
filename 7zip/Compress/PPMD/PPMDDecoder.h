// Compress/PPM/PPMDDecoder.h

#pragma once

#ifndef __COMPRESS_PPM_PPMD_DECODER_H
#define __COMPRESS_PPM_PPMD_DECODER_H

#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/OutBuffer.h"
#include "../RangeCoder/RangeCoder.h"

#include "PPMDDecode.h"

namespace NCompress {
namespace NPPMD {

class CDecoder : 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CMyUnknownImp
{
  NRangeCoder::CDecoder _rangeDecoder;

  COutBuffer _outStream;

  CDecodeInfo _info;

  BYTE _order;
  UINT32 _usedMemorySize;

public:

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties)

  /*
  void ReleaseStreams()
  {
    _rangeDecoder.ReleaseStream();
    _outStream.ReleaseStream();
  }
  */

  // STDMETHOD(Code)(UINT32 aNumBytes, UINT32 &aProcessedBytes);
  HRESULT Flush()
    { return _outStream.Flush(); }


  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);


  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

};

}}

#endif
