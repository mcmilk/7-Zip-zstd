// ImplodeDecoder.h

#ifndef __IMPLODE_DECODER_H
#define __IMPLODE_DECODER_H

#pragma once

#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../LZ/LZOutWindow.h"

#include "ImplodeHuffmanDecoder.h"

namespace NCompress {
namespace NImplode {
namespace NDecoder {

class CException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CException(ECauseType cause): m_Cause(cause) {}
};

class CCoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  NStream::NLSBF::CDecoder<CInBuffer> m_InBitStream;
  
  NImplode::NHuffman::CDecoder m_LiteralDecoder;
  NImplode::NHuffman::CDecoder m_LengthDecoder;
  NImplode::NHuffman::CDecoder m_DistanceDecoder;


  bool m_BigDictionaryOn;
  bool m_LiteralsOn;

  int m_NumDistanceLowDirectBits; 
  UINT32 m_MinMatchLength;

  void ReadLevelItems(NImplode::NHuffman::CDecoder &table, 
      BYTE *levels, int numLevelItems);
  void ReadTables();
  void DeCodeLevelTable(BYTE *newLevels, int numLevels);
public:
  CCoder();

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties)

  // STDMETHOD(ReleaseStreams)();
  // STDMETHOD(Code)(UINT32 aSize, UINT32 &aProcessedSize);
  HRESULT (Flush)() { return m_OutWindowStream.Flush(); }


  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetDecoderProperties
  // STDMETHOD(SetCoderProperties)(PROPVARIANT *aProperties, UINT32 aNumProperties);
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);
};

}}}

#endif
