// Bcj2Coder.h

#ifndef __COMPRESS_BCJ2_CODER_H
#define __COMPRESS_BCJ2_CODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "RangeCoderBit.h"

namespace NCompress {
namespace NBcj2 {

const unsigned kNumMoveBits = 5;

#ifndef EXTRACT_ONLY

class CEncoder:
  public ICompressCoder2,
  public CMyUnknownImp
{
  Byte *_buf;

  COutBuffer _mainStream;
  COutBuffer _callStream;
  COutBuffer _jumpStream;
  NRangeCoder::CEncoder _rc;
  NRangeCoder::CBitEncoder<kNumMoveBits> _statusEncoder[256 + 2];

  HRESULT Flush();

public:
  MY_UNKNOWN_IMP

  HRESULT CodeReal(ISequentialInStream **inStreams, const UInt64 **inSizes, UInt32 numInStreams,
      ISequentialOutStream **outStreams, const UInt64 **outSizes, UInt32 numOutStreams,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream **inStreams, const UInt64 **inSizes, UInt32 numInStreams,
      ISequentialOutStream **outStreams, const UInt64 **outSizes, UInt32 numOutStreams,
      ICompressProgressInfo *progress);
  
  CEncoder(): _buf(0) {};
  ~CEncoder();
};

#endif

class CDecoder:
  public ICompressCoder2,
  public ICompressSetBufSize,
  public CMyUnknownImp
{
  CInBuffer _mainStream;
  CInBuffer _callStream;
  CInBuffer _jumpStream;
  NRangeCoder::CDecoder _rc;
  NRangeCoder::CBitDecoder<kNumMoveBits> _statusDecoder[256 + 2];

  COutBuffer _outStream;
  UInt32 _inBufSizes[4];
  UInt32 _outBufSize;

public:
  MY_UNKNOWN_IMP1(ICompressSetBufSize);
  
  HRESULT CodeReal(ISequentialInStream **inStreams, const UInt64 **inSizes, UInt32 numInStreams,
      ISequentialOutStream **outStreams, const UInt64 **outSizes, UInt32 numOutStreams,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream **inStreams, const UInt64 **inSizes, UInt32 numInStreams,
      ISequentialOutStream **outStreams, const UInt64 **outSizes, UInt32 numOutStreams,
      ICompressProgressInfo *progress);

  STDMETHOD(SetInBufSize)(UInt32 streamIndex, UInt32 size);
  STDMETHOD(SetOutBufSize)(UInt32 streamIndex, UInt32 size);

  CDecoder();
};

}}

#endif
