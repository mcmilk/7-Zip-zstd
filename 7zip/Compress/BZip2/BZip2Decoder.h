// Compress/BZip2/Decoder.h

#pragma once

#ifndef __COMPRESS_BZIP2_DECODER_H
#define __COMPRESS_BZIP2_DECODER_H

#include "../../ICoder.h"
// #include "../../Interface/CompressInterface.h"
#include "Common/MyCom.h"

namespace NCompress {
namespace NBZip2 {

class CDecoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  BYTE *m_InBuffer;
  BYTE *m_OutBuffer;
public:
  CDecoder();
  ~CDecoder();

  MY_UNKNOWN_IMP

  HRESULT Flush();
  // void (ReleaseStreams)();

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};

}}

#endif
