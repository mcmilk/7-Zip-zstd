// ShrinkDecoder.h

#ifndef __SHRINK_DECODER_H
#define __SHRINK_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"

namespace NCompress {
namespace NShrink {

const int kNumMaxBits = 13;   
const UInt32 kNumItems = 1 << kNumMaxBits;   

class CDecoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  UInt16 _parents[kNumItems];
  Byte _suffixes[kNumItems];
  Byte _stack[kNumItems];
  bool _isFree[kNumItems];
  bool _isParent[kNumItems];
public:
  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream, ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
};

}}

#endif
