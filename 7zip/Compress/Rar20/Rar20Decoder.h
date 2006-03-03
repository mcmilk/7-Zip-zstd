// Rar20Decoder.h
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver

#ifndef __RAR20_DECODER_H
#define __RAR20_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/MSBFDecoder.h"
#include "../../Common/InBuffer.h"

#include "../LZ/LZOutWindow.h"
#include "../Huffman/HuffmanDecoder.h"

#include "Rar20Multimedia.h"
#include "Rar20Const.h"

namespace NCompress {
namespace NRar20 {

typedef NStream::NMSBF::CDecoder<CInBuffer> CBitDecoder;

const int kNumHuffmanBits = 15;

class CDecoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  CBitDecoder m_InBitStream;
  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize> m_MainDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kDistTableSize> m_DistDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kLenTableSize> m_LenDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kMMTableSize> m_MMDecoders[NMultimedia::kNumChanelsMax];
  NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  bool m_AudioMode;

  NMultimedia::CPredictor m_Predictor;
  int m_NumChannels;

  UInt32 m_RepDists[kNumRepDists];
  UInt32 m_RepDistPtr;

  UInt32 m_LastLength;
  
  Byte m_LastLevels[kMaxTableSize];

  UInt64 m_PackSize;
  bool m_IsSolid;

  void InitStructures();
  bool ReadTables();
  bool ReadLastTables();

public:
  CDecoder();

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

};

}}

#endif
