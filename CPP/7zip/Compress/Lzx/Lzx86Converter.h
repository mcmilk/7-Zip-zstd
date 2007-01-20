// Lzx/x86Converter.h

#ifndef __LZX_X86CONVERTER_H
#define __LZX_X86CONVERTER_H

#include "Common/MyCom.h"
#include "../../IStream.h"

namespace NCompress {
namespace NLzx {

const int kUncompressedBlockSize = 1 << 15;

class Cx86ConvertOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> m_Stream;
  UInt32 m_ProcessedSize;
  UInt32 m_Pos;
  UInt32 m_TranslationSize;
  bool m_TranslationMode;
  Byte m_Buffer[kUncompressedBlockSize];

  void MakeTranslation();
public:
  void SetStream(ISequentialOutStream *outStream) { m_Stream = outStream; }
  void ReleaseStream() { m_Stream.Release(); }
  void Init(bool translationMode, UInt32 translationSize)
  {
    m_TranslationMode = translationMode;
    m_TranslationSize = translationSize;
    m_ProcessedSize = 0;
    m_Pos = 0;
  }
  HRESULT Flush();

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

}}

#endif
