// Lzx86Converter.h

#ifndef __LZX_86_CONVERTER_H
#define __LZX_86_CONVERTER_H

#include "../../Common/MyCom.h"

#include "../IStream.h"

namespace NCompress {
namespace NLzx {

const unsigned kUncompressedBlockSize = (unsigned)1 << 15;

class Cx86ConvertOutStream:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  ISequentialOutStream *_stream;
  UInt32 _processedSize;
  UInt32 _pos;
  UInt32 _translationSize;
  bool _translationMode;
  Byte _buf[kUncompressedBlockSize];

  void MakeTranslation();
public:
  void SetStream(ISequentialOutStream *outStream) { _stream = outStream; }
  void Init(bool translationMode, UInt32 translationSize)
  {
    _translationMode = translationMode;
    _translationSize = translationSize;
    _processedSize = 0;
    _pos = 0;
  }
  HRESULT Flush();

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

}}

#endif
