// Rar15Decoder.h
// According to unRAR license, this code may not be used to develop 
// a program that creates RAR archives

#ifndef __RAR10_DECODER_H
#define __RAR10_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/MSBFDecoder.h"
#include "../../Common/InBuffer.h"

#include "../LZ/LZOutWindow.h"
#include "../Huffman/HuffmanDecoder.h"

namespace NCompress {
namespace NRar1 {

const UInt32 kNumRepDists = 4;

typedef NStream::NMSBF::CDecoder<CInBuffer> CBitDecoder;

class CDecoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
public:
  CLZOutWindow m_OutWindowStream;
  CBitDecoder m_InBitStream;

  UInt32 m_RepDists[kNumRepDists];
  UInt32 m_RepDistPtr;

  UInt32 LastDist;
  UInt32 LastLength;

  Int64 m_UnpackSize;
  bool m_IsSolid;

  UInt32 ReadBits(int numBits);
  HRESULT CopyBlock(UInt32 distance, UInt32 len);

  UInt32 DecodeNum(const UInt32 *posTab);
  HRESULT ShortLZ();
  HRESULT LongLZ();
  HRESULT HuffDecode();
  void GetFlagsBuf();
  void InitData();
  void InitHuff();
  void CorrHuff(UInt32 *CharSet, UInt32 *NumToPlace);
  void OldUnpWriteBuf();
  
  UInt32 ChSet[256],ChSetA[256],ChSetB[256],ChSetC[256];
  UInt32 Place[256],PlaceA[256],PlaceB[256],PlaceC[256];
  UInt32 NToPl[256],NToPlB[256],NToPlC[256];
  UInt32 FlagBuf,AvrPlc,AvrPlcB,AvrLn1,AvrLn2,AvrLn3;
  int Buf60,NumHuf,StMode,LCount,FlagsCnt;
  UInt32 Nhfb,Nlzb,MaxDist3;

  void InitStructures();

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
