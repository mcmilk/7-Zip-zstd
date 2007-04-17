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
  #ifndef NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
  #endif
  public CMyUnknownImp
{
  CRangeDecoder _rangeDecoder;

  COutBuffer _outStream;

  CDecodeInfo _info;

  Byte _order;
  UInt32 _usedMemorySize;

  int _remainLen;
  UInt64 _outSize;
  bool _outSizeDefined;
  UInt64 _processedSize;

  HRESULT CodeSpec(UInt32 num, Byte *memStream);
public:

  #ifndef NO_READ_FROM_CODER
  MY_UNKNOWN_IMP4(
      ICompressSetDecoderProperties2, 
      ICompressSetInStream, 
      ICompressSetOutStreamSize, 
      ISequentialInStream)
  #else
  MY_UNKNOWN_IMP1(
      ICompressSetDecoderProperties2)
  #endif

  void ReleaseStreams()
  {
    ReleaseInStream();
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

  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);

  #ifndef NO_READ_FROM_CODER
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  CDecoder(): _outSizeDefined(false) {}

};

}}

#endif
