// Implode/Decoder.h

#ifndef __IMPLODE_DECODE_H
#define __IMPLODE_DECODE_H

#pragma once

#include "Interface/ICoder.h"
#include "../../Interface/CompressInterface.h"

#include "Stream/WindowOut.h"
#include "Stream/LSBFDecoder.h"
#include "Stream/InByte.h"

#include "HuffmanDecoder.h"

// {23170F69-40C1-278B-0401-060000000000}
DEFINE_GUID(CLSID_CCompressImplodeDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NImplode {
namespace NDecoder {

class CException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CException(ECauseType aCause): m_Cause(aCause) {}
};

typedef NStream::NLSBF::CDecoder<NStream::CInByte> CInBit;

class CCoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressImplodeDecoder>
{
  NStream::NWindow::COut m_OutWindowStream;
  CInBit m_InBitStream;
  
  NImplode::NHuffman::CDecoder m_LiteralDecoder;
  NImplode::NHuffman::CDecoder m_LengthDecoder;
  NImplode::NHuffman::CDecoder m_DistanceDecoder;


  bool m_BigDictionaryOn;
  bool m_LiteralsOn;

  int m_NumDistanceLowDirectBits; 
  UINT32 m_MinMatchLength;

  void ReadLevelItems(NImplode::NHuffman::CDecoder &aTable, 
      BYTE *aLevels, int aNumLevelItems);
  void ReadTables();
  void DeCodeLevelTable(BYTE *aNewLevels, int aNumLevels);
public:
  CCoder();
  ~CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CEncoder, 
    // TEXT("Compress.ImplodeDecoder.1"), TEXT("Compress.ImplodeDecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Init)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream);
  STDMETHOD(ReleaseStreams)();
  // STDMETHOD(Code)(UINT32 aSize, UINT32 &aProcessedSize);
  STDMETHOD(Flush)();

  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // ICompressSetDecoderProperties
  // STDMETHOD(SetCoderProperties)(PROPVARIANT *aProperties, UINT32 aNumProperties);
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *anInStream);
};

}}

#endif
