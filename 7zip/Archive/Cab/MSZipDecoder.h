// Archive/Cab/MSZipDecoder.h

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
  void InitMain(Byte reservedSize, UInt32 aNumBlocks)
  {
    m_Stream.Init(reservedSize, aNumBlocks);
    Init();
  }
  HRESULT ReadBlock(UInt32 &uncompressedSize, bool &dataAreCorrect)
  {
    return m_Stream.ReadBlock(uncompressedSize, dataAreCorrect);
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
  UInt32 m_StoredBlockSize;

  Byte m_ReservedSize;
  UInt32 m_NumInDataBlocks;

  void DeCodeLevelTable(Byte *newLevels, int numLevels);
  void ReadTables();
public:
  CDecoder();

  MY_UNKNOWN_IMP

  HRESULT Flush();
  // void ReleaseStreams();

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  void SetParams(Byte reservedSize, UInt32 numInDataBlocks) 
  { 
    m_ReservedSize = reservedSize;
    m_NumInDataBlocks = numInDataBlocks;
  }

};

}}}

#endif