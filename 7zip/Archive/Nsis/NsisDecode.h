// NsisDecode.h

#ifndef __NSIS_DECODE_H
#define __NSIS_DECODE_H

#include "../../IStream.h"

#include "../Common/CoderLoader.h"

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
  CCoderLibraries _libraries;

  CMyComPtr<ISequentialInStream> _filterInStream;
  CMyComPtr<ISequentialInStream> _codecInStream;
  CMyComPtr<ISequentialInStream> _decoderInStream;

public:
  CDecoder();
  void Release()
  {
    _filterInStream.Release();
    _codecInStream.Release();
    _decoderInStream.Release();
  }
  HRESULT Init(IInStream *inStream, NMethodType::EEnum method, bool thereIsFilterFlag, bool &useFilter);
  HRESULT Read(void *data, UInt32 size, UInt32 *processedSize);
};

}}

#endif
