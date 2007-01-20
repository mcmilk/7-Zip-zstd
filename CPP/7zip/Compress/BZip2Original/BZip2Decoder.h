// Compress/BZip2/Decoder.h

#ifndef __COMPRESS_BZIP2_DECODER_H
#define __COMPRESS_BZIP2_DECODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"

namespace NCompress {
namespace NBZip2 {

class CDecoder :
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  public CMyUnknownImp
{
  Byte *m_InBuffer;
  UInt64 m_InSize;
public:
  CDecoder(): m_InBuffer(0), m_InSize(0) {};
  ~CDecoder();

  MY_UNKNOWN_IMP1(ICompressGetInStreamProcessedSize)

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);
};

}}

#endif
