// Archive/Cab/LZXi86Converter.h

#ifndef __ARCHIVE_CAB_LZXI86CONVERTER_H
#define __ARCHIVE_CAB_LZXI86CONVERTER_H

#include "Common/MyCom.h"
#include "../../IStream.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

const int kUncompressedBlockSize = 1 << 15;

class Ci86TranslationOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  bool m_TranslationMode;
  CMyComPtr<ISequentialOutStream> m_Stream;
  UInt32 m_ProcessedSize;
  Byte m_Buffer[kUncompressedBlockSize];
  UInt32 m_Pos;
  UInt32 m_TranslationSize;

  void MakeTranslation();
public:
  Ci86TranslationOutStream(): m_Pos(0) {}
  ~Ci86TranslationOutStream() { Flush(); }

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UInt32 size, UInt32 *processedSize);
public:
  void Init(ISequentialOutStream *outStream, bool translationMode, UInt32 translationSize)
  {
    m_Stream = outStream;
    m_TranslationMode = translationMode;
    m_TranslationSize = translationSize;
    m_ProcessedSize = 0;
    m_Pos = 0;
  }
  void ReleaseStream() { m_Stream.Release(); }
  HRESULT Flush();
};

}}}

#endif
