// Compress/BZip2/Encoder.h

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"
#include "../../Common/MSBFEncoder.h"
#include "../../Common/InBuffer.h"
#include "../../Common/OutBuffer.h"
#include "../Huffman/HuffmanEncoder.h"
#include "../BWT/BlockSort.h"
#include "BZip2Const.h"
#include "BZip2CRC.h"
 
namespace NCompress {
namespace NBZip2 {

class CMsbfEncoderTemp
{
  UInt32 m_Pos;
  int m_BitPos;
  Byte m_CurByte;
  Byte *Buffer;
public:
  void SetStream(Byte *buffer) { Buffer = buffer;  }
  Byte *GetStream() const { return Buffer; }

  void Init()
  {
    m_Pos = 0;
    m_BitPos = 8; 
    m_CurByte = 0;
  }

  void Flush()
  {
    if(m_BitPos < 8)
      WriteBits(0, m_BitPos);
  }

  void WriteBits(UInt32 value, int numBits)
  {
    while(numBits > 0)
    {
      int numNewBits = MyMin(numBits, m_BitPos);
      numBits -= numNewBits;
      
      m_CurByte <<= numNewBits;
      UInt32 newBits = value >> numBits;
      m_CurByte |= Byte(newBits);
      value -= (newBits << numBits);
      
      m_BitPos -= numNewBits;
      
      if (m_BitPos == 0)
      {
       Buffer[m_Pos++] = m_CurByte;
        m_BitPos = 8;
      }
    }
  }
  
  UInt32 GetBytePos() const { return m_Pos ; }
  UInt32 GetPos() const { return m_Pos * 8 + (8 - m_BitPos); }
  Byte GetCurByte() const { return m_CurByte; }
  void SetPos(UInt32 bitPos)
  { 
    m_Pos = bitPos / 8;
    m_BitPos = 8 - (bitPos & 7); 
  }
  void SetCurState(UInt32 bitPos, Byte curByte)
  { 
    m_BitPos = 8 - bitPos; 
    m_CurByte = curByte; 
  }
};

class CEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties, 
  public CMyUnknownImp
{
  Byte *m_Block;
  CInBuffer m_InStream;
  NStream::NMSBF::CEncoder<COutBuffer> m_OutStream;
  CMsbfEncoderTemp *m_OutStreamCurrent;
  CBlockSorter m_BlockSorter;

  bool m_NeedHuffmanCreate;
  NCompression::NHuffman::CEncoder m_HuffEncoders[kNumTablesMax];

  Byte *m_MtfArray;
  Byte *m_TempArray;

  Byte m_Selectors[kNumSelectorsMax];

  UInt32 m_BlockSizeMult;
  UInt32 m_NumPasses;
  bool m_OptimizeNumTables;

  UInt32 ReadRleBlock(Byte *buffer);

  void WriteBits2(UInt32 value, UInt32 numBits);
  void WriteByte2(Byte b);
  void WriteBit2(bool v);
  void WriteCRC2(UInt32 v);
  
  void WriteBits(UInt32 value, UInt32 numBits);
  void WriteByte(Byte b);
  void WriteBit(bool v);
  void WriteCRC(UInt32 v);

  void EncodeBlock(Byte *block, UInt32 blockSize);
  UInt32 EncodeBlockWithHeaders(Byte *block, UInt32 blockSize);
  void EncodeBlock2(CBZip2CombinedCRC &combinedCRC, Byte *block, UInt32 blockSize, UInt32 numPasses);
  void EncodeBlock3(CBZip2CombinedCRC &combinedCRC, UInt32 blockSize);

public:
  CEncoder();
  ~CEncoder();

  HRESULT Flush() { return m_OutStream.Flush(); }
  
  void ReleaseStreams()
  {
    m_InStream.ReleaseStream();
    m_OutStream.ReleaseStream();
  }

  class CFlusher
  {
    CEncoder *_coder;
  public:
    bool NeedFlush;
    CFlusher(CEncoder *coder): _coder(coder), NeedFlush(true) {}
    ~CFlusher() 
    { 
      if (NeedFlush)
        _coder->Flush();
      _coder->ReleaseStreams(); 
    }
  };

  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)

  HRESULT CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
};

}}

#endif
