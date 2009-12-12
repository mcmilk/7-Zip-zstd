// BZip2Encoder.h

#ifndef __COMPRESS_BZIP2_ENCODER_H
#define __COMPRESS_BZIP2_ENCODER_H

#include "../../Common/Defs.h"
#include "../../Common/MyCom.h"

#ifndef _7ZIP_ST
#include "../../Windows/Synchronization.h"
#include "../../Windows/Thread.h"
#endif

#include "../ICoder.h"

#include "../Common/InBuffer.h"
#include "../Common/OutBuffer.h"

#include "BitmEncoder.h"
#include "BZip2Const.h"
#include "BZip2Crc.h"

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
    if (m_BitPos < 8)
      WriteBits(0, m_BitPos);
  }

  void WriteBits(UInt32 value, int numBits)
  {
    while (numBits > 0)
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
    m_BitPos = 8 - ((int)bitPos & 7);
  }
  void SetCurState(int bitPos, Byte curByte)
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

  UInt32 m_BlockIndex;

  void WriteBits2(UInt32 value, UInt32 numBits);
  void WriteByte2(Byte b);
  void WriteBit2(bool v);
  void WriteCrc2(UInt32 v);

  void EncodeBlock(const Byte *block, UInt32 blockSize);
  UInt32 EncodeBlockWithHeaders(const Byte *block, UInt32 blockSize);
  void EncodeBlock2(const Byte *block, UInt32 blockSize, UInt32 numPasses);
public:
  bool m_OptimizeNumTables;
  CEncoder *Encoder;
  #ifndef _7ZIP_ST
  NWindows::CThread Thread;

  NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;

  // it's not member of this thread. We just need one event per thread
  NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;

  UInt64 m_PackSize;

  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  HRESULT Create();
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
  #ifndef _7ZIP_ST
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
  CBitmEncoder<COutBuffer> m_OutStream;
  UInt32 NumPasses;
  CBZip2CombinedCrc CombinedCrc;

  #ifndef _7ZIP_ST
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
  void WriteCrc(UInt32 v);

  #ifndef _7ZIP_ST
  HRESULT Create();
  void Free();
  #endif

public:
  CEncoder();
  #ifndef _7ZIP_ST
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
    CFlusher(CEncoder *coder): _coder(coder) {}
    ~CFlusher()
    {
      _coder->ReleaseStreams();
    }
  };

  #ifndef _7ZIP_ST
  MY_UNKNOWN_IMP2(ICompressSetCoderMt, ICompressSetCoderProperties)
  #else
  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
  #endif

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);

  #ifndef _7ZIP_ST
  STDMETHOD(SetNumberOfThreads)(UInt32 numThreads);
  #endif
};

}}

#endif
