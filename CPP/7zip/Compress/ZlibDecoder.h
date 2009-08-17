// ZlibDecoder.h

#ifndef __ZLIB_DECODER_H
#define __ZLIB_DECODER_H

#include "DeflateDecoder.h"

namespace NCompress {
namespace NZlib {

const UInt32 ADLER_INIT_VAL = 1;

class COutStreamWithAdler:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UInt32 _adler;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _adler = ADLER_INIT_VAL; }
  UInt32 GetAdler() const { return _adler; }
};

class CDecoder:
  public ICompressCoder,
  public CMyUnknownImp
{
  COutStreamWithAdler *AdlerSpec;
  CMyComPtr<ISequentialOutStream> AdlerStream;
  
  NCompress::NDeflate::NDecoder::CCOMCoder *DeflateDecoderSpec;
  CMyComPtr<ICompressCoder> DeflateDecoder;
public:
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  UInt64 GetInputProcessedSize() const { return DeflateDecoderSpec->GetInputProcessedSize() + 2; }

  MY_UNKNOWN_IMP
};

}}

#endif
