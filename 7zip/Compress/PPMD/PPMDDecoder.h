// Compress/PPM/PPMDDecoder.h

#ifndef __COMPRESS_PPMD_DECODER_H
#define __COMPRESS_PPMD_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/OutBuffer.h"
#include "../RangeCoder/RangeCoder.h"

#include "PPMDDecode.h"

namespace NCompress {
namespace NPPMD {

class CDecoder : 
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  NRangeCoder::CDecoder _rangeDecoder;

  COutBuffer _outStream;

  CDecodeInfo _info;

  Byte _order;
  UInt32 _usedMemorySize;

public:

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  void ReleaseStreams()
  {
    _rangeDecoder.ReleaseStream();
    _outStream.ReleaseStream();
  }

  HRESULT Flush() { return _outStream.Flush(); }

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);


  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}}

#endif
