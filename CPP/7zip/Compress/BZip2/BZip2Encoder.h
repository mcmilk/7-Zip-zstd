// Compress/BZip2/Encoder.h

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"
#include "../../Common/MSBFEncoder.h"
#include "../../Common/InBuffer.h"
#include "../../Common/OutBuffer.h"
#include "BZip2Const.h"
#include "BZip2CRC.h"

#ifdef COMPRESS_BZIP2_MT
#include "../../../Windows/Thread.h"
#include "../../../Windows/Synchronization.h"
#endif

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

class CEncoder;

const int kNumPassesMax = 10;

class CThreadInfo
{
public:
  Byte *m_Block;
private:
  Byte *m_MtfArray;
  Byte *m_TempArray;
  UInt32 *m_BlockSorterIndex;

  CMsbfEncoderTemp *m_OutStreamCurrent;

  Byte Lens[kNumTablesMax][kMaxAlphaSize];
  UInt32 Freqs[kNumTablesMax][kMaxAlphaSize];
  UInt32 Codes[kNumTablesMax][kMaxAlphaSize];

  Byte m_Selectors[kNumSelectorsMax];

  UInt32 m_CRCs[1 << kNumPassesMax];
  UInt32 m_NumCrcs;

  int m_BlockIndex;

  void WriteBits2(UInt32 value, UInt32 numBits);
  void WriteByte2(Byte b);
  void WriteBit2(bool v);
  void WriteCRC2(UInt32 v);

  void EncodeBlock(const Byte *block, UInt32 blockSize);
  UInt32 EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize);
  void EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses);
public:
  bool m_OptimizeNumTables;
  CEncoder *Encoder;
  #ifdef COMPRESS_BZIP2_MT
  NWindows::CThread Thread;

  NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;

  // it's not member of this thread. We just need one event per thread
  NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;

  UInt64 m_PackSize;

  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  HRes Create();
  void FinishStream(bool needLeave);
  DWORD ThreadFunc();
  #endif

  CThreadInfo(): m_BlockSorterIndex(0), m_Block(0) {}
  ~CThreadInfo() { Free(); }
  bool Alloc();
  void Free();

  HRESULT EncodeBlock3(UInt32 blockSize);
};

class CEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties, 
  #ifdef COMPRESS_BZIP2_MT
  public ICompressSetCoderMt,
  #endif
  public CMyUnknownImp
{
  UInt32 m_BlockSizeMult;
  bool m_OptimizeNumTables;

  UInt32 m_NumPassesPrev;

  UInt32 m_NumThreadsPrev;
public:
  CInBuffer m_InStream;
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  NStream::NMSBF::CEncoder<COutBuffer> m_OutStream;
  UInt32 NumPasses;
  CBZip2CombinedCRC CombinedCRC;

  #ifdef COMPRESS_BZIP2_MT
  CThreadInfo *ThreadsInfo;
  NWindows::NSynchronization::CManualResetEvent CanProcessEvent;
  NWindows::NSynchronization::CCriticalSection CS;
  UInt32 NumThreads;
  bool MtMode;
  UInt32 NextBlockIndex;

  bool CloseThreads;
  bool StreamWasFinished;
  NWindows::NSynchronization::CManualResetEvent CanStartWaitingEvent;

  HRESULT Result;
  ICompressProgressInfo *Progress;
  #else
  CThreadInfo ThreadsInfo;
  #endif

  UInt32 ReadRleBlock(Byte *buffer);
  void WriteBytes(const Byte *data, UInt32 sizeInBits, Byte lastByte);

  void WriteBits(UInt32 value, UInt32 numBits);
  void WriteByte(Byte b);
  void WriteBit(bool v);
  void WriteCRC(UInt32 v);

  #ifdef COMPRESS_BZIP2_MT
  HRes Create();
  void Free();
  #endif

public:
  CEncoder();
  #ifdef COMPRESS_BZIP2_MT
  ~CEncoder();
  #endif

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

  #ifdef COMPRESS_BZIP2_MT
  MY_UNKNOWN_IMP2(ICompressSetCoderMt, ICompressSetCoderProperties)
  #else
  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
  #endif

  HRESULT CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);

  #ifdef COMPRESS_BZIP2_MT
  STDMETHOD(SetNumberOfThreads)(UInt32 numThreads);
  #endif
};

}}

#endif
