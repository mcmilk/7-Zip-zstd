// PpmdEncoder.h
// 2009-05-30 : Igor Pavlov : Public domain

#ifndef __COMPRESS_PPMD_ENCODER_H
#define __COMPRESS_PPMD_ENCODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "PpmdEncode.h"
#include "RangeCoder.h"

namespace NCompress {
namespace NPpmd {

class CEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
public:
  CInBuffer _inStream;

  NRangeCoder::CEncoder _rangeEncoder;

  CEncodeInfo _info;
  UInt32 _usedMemorySize;
  Byte _order;

  HRESULT Flush()
  {
    _rangeEncoder.FlushData();
    return _rangeEncoder.FlushStream();
  }

  void ReleaseStreams()
  {
    _inStream.ReleaseStream();
    _rangeEncoder.ReleaseStream();
  }

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

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

public:

  MY_UNKNOWN_IMP2(
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties)

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);

  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();
};

}}

#endif
