// Compress/CopyCoder.h

// #pragma once

#ifndef __COMPRESS_COPYCODER_H
#define __COMPRESS_COPYCODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"


namespace NCompress {

class CCopyCoder: 
  public ICompressCoder,
  public CMyUnknownImp
{
  BYTE *_buffer;
public:
  UINT64 TotalSize;
  CCopyCoder();
  ~CCopyCoder();

  MY_UNKNOWN_IMP

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
};

}

#endif
