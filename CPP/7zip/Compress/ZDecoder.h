// ZDecoder.h

#ifndef __COMPRESS_Z_DECODER_H
#define __COMPRESS_Z_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCompress {
namespace NZ {

class CDecoder:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  UInt16 *_parents;
  Byte *_suffixes;
  Byte *_stack;
  Byte _properties;
  int _numMaxBits;

public:
  CDecoder(): _parents(0), _suffixes(0), _stack(0), _properties(0), _numMaxBits(0) {};
  ~CDecoder();
  void Free();

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}}

#endif
