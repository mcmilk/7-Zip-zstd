// Archive/Cab/LZXDecoder.h

#pragma once

#ifndef __ARCHIVE_CAB_LZXDECODER_H
#define __ARCHIVE_CAB_LZXDECODER_H

#include "Interface/ICoder.h"

#include "Compression/HuffmanDecoder.h"

#include "Stream/WindowOut.h"

#include "LZXExtConst.h"
#include "LZXBitDecoder.h"

#include "LZXi86Converter.h"

// #include "../../../Projects/CompressInterface/CompressInterface.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

typedef NCompression::NHuffman::CDecoder<kNumHuffmanBits> CMSBFHuffmanDecoder;

class CDecoder : 
  public ICompressCoder,
  public CComObjectRoot
{
  NStream::NWindow::COut m_OutWindowStream;
  NBitStream::CDecoder m_InBitStream;

  CMSBFHuffmanDecoder m_MainDecoder;
  CMSBFHuffmanDecoder m_LenDecoder; // for matches with repeated offsets
  CMSBFHuffmanDecoder m_AlignDecoder; // for matches with repeated offsets
  CMSBFHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  UINT32 m_RepDistances[kNumRepDistances];

  BYTE m_LastByteLevels[256];
  BYTE m_LastPosLenLevels[kNumPosSlotLenSlotSymbols];
  BYTE m_LastLenLevels[kNumLenSymbols];

  UINT32 m_DictionarySizePowerOf2;
  UINT32 m_NumPosSlots;
  UINT32 m_NumPosLenSlots;

  // bool m_i86PreprocessingUsed;
  // UINT32 m_i86TranslationSize;
  
  bool m_UncompressedBlock;
  bool m_AlignIsUsed;

  UINT32 m_UnCompressedBlockSize;

  CComObjectNoLock<Ci86TranslationOutStream> *m_i86TranslationOutStreamSpec;
  CComPtr<ISequentialOutStream> m_i86TranslationOutStream;

  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;

  void ReadTable(BYTE *aLastLevels, BYTE *aNewLevels, UINT32 aNumSymbols);
  void ReadTables();
  void ClearPrevLeveles();

public:
  CDecoder();

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

  // DECLARE_REGISTRY(CDecoder, "Compress.LZXDecoder.1", "Compress.LZXDecoder", 0, THREADFLAGS_APARTMENT)
  DECLARE_NO_REGISTRY()


  void ReleaseStreams();
  // STDMETHOD(Code)(UINT32 aNumBytes, UINT32 &aProcessedBytes);
  STDMETHOD(Flush)();

  // ICompressCoder interface
  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  void SetParams(BYTE aReservedSize, UINT32 aNumInDataBlocks, UINT32 aDictionarySizePowerOf2) 
    { 
      m_ReservedSize = aReservedSize;
      m_NumInDataBlocks = aNumInDataBlocks;
      m_DictionarySizePowerOf2 = aDictionarySizePowerOf2; 
      if (aDictionarySizePowerOf2 < 20)
        m_NumPosSlots = 30 + (aDictionarySizePowerOf2 - 15) * 2;
      else if (aDictionarySizePowerOf2 == 20)
        m_NumPosSlots = 42;
      else
        m_NumPosSlots = 50;
      m_NumPosLenSlots = m_NumPosSlots * kNumLenSlots;
    }
};

}}}

#endif