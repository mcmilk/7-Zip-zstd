// Compress/BZip2/Encoder.h

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"

namespace NCompress {
namespace NBZip2 {

class CEncoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  Byte *m_InBuffer;
public:
  CEncoder(): m_InBuffer(0) {};
  ~CEncoder();

  MY_UNKNOWN_IMP

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
};

}}

#endif
