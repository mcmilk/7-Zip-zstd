// Compress/BZip2/Decoder.h

#ifndef __COMPRESS_BZIP2_DECODER_H
#define __COMPRESS_BZIP2_DECODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"

namespace NCompress {
namespace NBZip2 {

class CDecoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  Byte *m_InBuffer;
public:
  CDecoder(): m_InBuffer(0) {};
  ~CDecoder();

  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
};

}}

#endif
