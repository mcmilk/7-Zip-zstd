// ZDecoder.h

#ifndef __COMPRESS_ZDECODER_H
#define __COMPRESS_ZDECODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"

namespace NCompress {
namespace NZ {

class CDecoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  BYTE _properties;
  int _numMaxBits;
  UInt16 *_parents;
  Byte *_suffixes;
  Byte *_stack;

public:
  CDecoder(): _properties(0), _numMaxBits(0), _parents(0), _suffixes(0), _stack(0) {};
  ~CDecoder();
  void Free();
  bool Alloc(size_t numItems);

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}}

#endif
