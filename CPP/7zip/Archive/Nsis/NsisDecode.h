// NsisDecode.h

#ifndef __NSIS_DECODE_H
#define __NSIS_DECODE_H

#include "../../IStream.h"

#include "../../Common/CreateCoder.h"

namespace NArchive {
namespace NNsis {

namespace NMethodType
{
  enum EEnum
  {
    kCopy,
    kDeflate,
    kBZip2,
    kLZMA
  };
}

class CDecoder
{
  NMethodType::EEnum _method;

  CMyComPtr<ISequentialInStream> _filterInStream;
  CMyComPtr<ISequentialInStream> _codecInStream;
  CMyComPtr<ISequentialInStream> _decoderInStream;

public:
  void Release()
  {
    _filterInStream.Release();
    _codecInStream.Release();
    _decoderInStream.Release();
  }
  HRESULT Init(
      DECL_EXTERNAL_CODECS_LOC_VARS
      IInStream *inStream, NMethodType::EEnum method, bool thereIsFilterFlag, bool &useFilter);
  HRESULT Read(void *data, size_t *processedSize);
};

}}

#endif
