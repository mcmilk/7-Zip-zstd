// Archive/Cab/LZXDecoder.h

#ifndef __ARCHIVE_CAB_LZXDECODER_H
#define __ARCHIVE_CAB_LZXDECODER_H

#include "../../ICoder.h"

#include "../../Compress/Huffman/HuffmanDecoder.h"
#include "../../Compress/LZ/LZOutWindow.h"

#include "LZXExtConst.h"
#include "LZXBitDecoder.h"

#include "LZXi86Converter.h"
#include "LZXConst.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

const int kMainTableSize = 256 + kNumPosSlotLenSlotSymbols;

class CDecoder : 
  public ICompressCoder,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  NBitStream::CDecoder m_InBitStream;

  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize> m_MainDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kNumLenSymbols> m_LenDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kAlignTableSize> m_AlignDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  UInt32 m_RepDistances[kNumRepDistances];

  Byte m_LastByteLevels[256];
  Byte m_LastPosLenLevels[kNumPosSlotLenSlotSymbols];
  Byte m_LastLenLevels[kNumLenSymbols];

  UInt32 m_DictionarySizePowerOf2;
  UInt32 m_NumPosSlots;
  UInt32 m_NumPosLenSlots;

  // bool m_i86PreprocessingUsed;
  // UInt32 m_i86TranslationSize;
  
  bool m_UncompressedBlock;
  bool m_AlignIsUsed;

  UInt32 m_UnCompressedBlockSize;

  Ci86TranslationOutStream *m_i86TranslationOutStreamSpec;
  CMyComPtr<ISequentialOutStream> m_i86TranslationOutStream;

  Byte m_ReservedSize;
  UInt32 m_NumInDataBlocks;

  void ReadTable(Byte *lastLevels, Byte *newLevels, UInt32 numSymbols);
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
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  void SetParams(Byte reservedSize, UInt32 numInDataBlocks, 
      UInt32 dictionarySizePowerOf2) 
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