// LzmaFiltersDecode.h

#ifndef __LZMA_FILTERS_DECODE_H
#define __LZMA_FILTERS_DECODE_H

#include "../../Common/CreateCoder.h"

#include "LzmaItem.h"

namespace NArchive {
namespace NLzma {

class CDecoder
{
  CMyComPtr<ICompressCoder> _lzmaDecoder;
  CMyComPtr<ISequentialOutStream> _bcjStream;
public:
  HRESULT Code(DECL_EXTERNAL_CODECS_LOC_VARS
      const CHeader &block,
      ISequentialInStream *inStream, ISequentialOutStream *outStream, 
      UInt64 *inProcessedSize, ICompressProgressInfo *progress);
};

}}

#endif
