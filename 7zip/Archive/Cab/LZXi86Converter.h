// Archive/Cab/LZXi86Converter.h

#pragma once

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
  UINT32 m_ProcessedSize;
  BYTE m_Buffer[kUncompressedBlockSize];
  UINT32 m_Pos;
  UINT32 m_TranslationSize;

  INT32 ConvertAbsoluteToOffset(INT32 aPos, INT32 anAbsoluteValue);
  void MakeTranslation();
public:
  Ci86TranslationOutStream();
  ~Ci86TranslationOutStream();

  MY_UNKNOWN_IMP

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
public:
  void Init(ISequentialOutStream *aStream, bool aTranslationMode, UINT32 aTranslationSize);
  HRESULT Flush();
  void ReleaseStream();
};

}}}

#endif