// Archive/Cab/LZXDecoder.h

#pragma once

#ifndef __ARCHIVE_CAB_LZXDECODER_H
#define __ARCHIVE_CAB_LZXDECODER_H

#include "../../ICoder.h"

#include "../../Compress/Huffman/HuffmanDecoder.h"
#include "../../Compress/LZ/LZOutWindow.h"

#include "LZXExtConst.h"
#include "LZXBitDecoder.h"

#include "LZXi86Converter.h"

// #include "../../../Projects/CompressInterface/CompressInterface.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

typedef NCompress::NHuffman::CDecoder<kNumHuffmanBits> CMSBFHuffmanDecoder;

class CDecoder : 
  public ICompressCoder,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
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

  Ci86TranslationOutStream *m_i86TranslationOutStreamSpec;
  CMyComPtr<ISequentialOutStream> m_i86TranslationOutStream;

  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;

  void ReadTable(BYTE *lastLevels, BYTE *newLevels, UINT32 numSymbols);
  void ReadTables();
  void ClearPrevLeveles();

public:
  CDecoder();

  MY_UNKNOWN_IMP

  void ReleaseStreams();
  STDMETHOD(Flush)();

  // ICompressCoder interface
  STDMETHOD(Code)(ISequentialInStream *inStream, 
      ISequentialOutStream *outStream, 
      const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  void SetParams(BYTE reservedSize, UINT32 numInDataBlocks, 
      UINT32 dictionarySizePowerOf2) 
    { 
      m_ReservedSize = reservedSize;
      m_NumInDataBlocks = numInDataBlocks;
      m_DictionarySizePowerOf2 = dictionarySizePowerOf2; 
      if (dictionarySizePowerOf2 < 20)
        m_NumPosSlots = 30 + (dictionarySizePowerOf2 - 15) * 2;
      else if (dictionarySizePowerOf2 == 20)
        m_NumPosSlots = 42;
      else
        m_NumPosSlots = 50;
      m_NumPosLenSlots = m_NumPosSlots * kNumLenSlots;
    }
};

}}}

#endif