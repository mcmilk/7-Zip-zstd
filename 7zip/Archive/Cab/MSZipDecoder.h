// Archive/Cab/MSZipDecoder.h

#pragma once

#ifndef __ARCHIVE_CAB_DECODER_H
#define __ARCHIVE_CAB_DECODER_H

#include "Common/MyCom.h"
#include "../../ICoder.h"
#include "../../Common/LSBFDecoder.h"
#include "../../Compress/Huffman/HuffmanDecoder.h"
#include "../../Compress/LZ/LZOutWindow.h"

#include "CabInBuffer.h"
#include "MSZipExtConst.h"

namespace NArchive {
namespace NCab {
namespace NMSZip {

class CDecoderException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CDecoderException(ECauseType aCause): m_Cause(aCause) {}
};

class CMSZipBitDecoder: public NStream::NLSBF::CDecoder<NCab::CInBuffer>
{
public:
  void InitMain(ISequentialInStream *aStream, BYTE aReservedSize, UINT32 aNumBlocks)
  {
    m_Stream.Init(aStream, aReservedSize, aNumBlocks);
    Init();
  }
  HRESULT ReadBlock(UINT32 &anUncompressedSize, bool &aDataAreCorrect)
  {
    return m_Stream.ReadBlock(anUncompressedSize, aDataAreCorrect);
  }
};

typedef NCompress::NHuffman::CDecoder<kNumHuffmanBits> CHuffmanDecoder;

class CDecoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  CMSZipBitDecoder m_InBitStream;
  CHuffmanDecoder m_MainDecoder;
  CHuffmanDecoder m_DistDecoder;
  CHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  bool m_FinalBlock;
  bool m_StoredMode;
  UINT32 m_StoredBlockSize;

  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;

  void DeCodeLevelTable(BYTE *newLevels, int numLevels);
  void ReadTables();
public:
  CDecoder();

  MY_UNKNOWN_IMP

  HRESULT Flush();
  // void ReleaseStreams();

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  void SetParams(BYTE aReservedSize, UINT32 aNumInDataBlocks) 
  { 
    m_ReservedSize = aReservedSize;
    m_NumInDataBlocks = aNumInDataBlocks;
  }

};

}}}

#endif