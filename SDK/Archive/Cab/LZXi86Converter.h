// Archive/Cab/LZXi86Converter.h

#pragma once

#ifndef __ARCHIVE_CAB_LZXI86CONVERTER_H
#define __ARCHIVE_CAB_LZXI86CONVERTER_H

#include "Interface/IInOutStreams.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

const kUncompressedBlockSize = 1 << 15;

class Ci86TranslationOutStream: 
  public ISequentialOutStream,
  public CComObjectRoot
{
  bool m_TranslationMode;
  CComPtr<ISequentialOutStream> m_Stream;
  UINT32 m_ProcessedSize;
  BYTE m_Buffer[kUncompressedBlockSize];
  UINT32 m_Pos;
  UINT32 m_TranslationSize;

  INT32 ConvertAbsoluteToOffset(INT32 aPos, INT32 anAbsoluteValue);
  void MakeTranslation();
public:
  Ci86TranslationOutStream();
  ~Ci86TranslationOutStream();

BEGIN_COM_MAP(Ci86TranslationOutStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(Ci86TranslationOutStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
public:
  void Init(ISequentialOutStream *aStream, bool aTranslationMode, UINT32 aTranslationSize);
  HRESULT Flush();
  void ReleaseStream();
};

}}}

#endif