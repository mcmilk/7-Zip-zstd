// Compress/BZip2/Encoder.h

#pragma once

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "Common/MyCom.h"
#include "../../ICoder.h"

namespace NCompress {
namespace NBZip2 {

class CEncoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  BYTE *m_InBuffer;
  BYTE *m_OutBuffer;

public:
  CEncoder();
  ~CEncoder();

  MY_UNKNOWN_IMP

  // STDMETHOD(ReleaseStreams)();

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};

}}

#endif