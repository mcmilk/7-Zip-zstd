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

  Int32 ConvertAbsoluteToOffset(Int32 aPos, Int32 anAbsoluteValue);
  void MakeTranslation();
public:
  Ci86TranslationOutStream();
  ~Ci86TranslationOutStream();

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *aData, UInt32 aSize, UInt32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UInt32 aSize, UInt32 *aProcessedSize);
public:
  void Init(ISequentialOutStream *aStream, bool aTranslationMode, UInt32 aTranslationSize);
  HRESULT Flush();
  void ReleaseStream();
};

}}}

#endif