// Compress/BZip2/Decoder.h

#ifndef __COMPRESS_BZIP2_DECODER_H
#define __COMPRESS_BZIP2_DECODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"
#include "../../Common/MSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../../Common/OutBuffer.h"
#include "../Huffman/HuffmanDecoder.h"
#include "BZip2Const.h"
#include "BZip2CRC.h"

#ifdef COMPRESS_BZIP2_MT
#include "../../../Windows/Thread.h"
#include "../../../Windows/Synchronization.h"
#endif

#if _MSC_VER >= 1300
#define NO_INLINE __declspec(noinline) __fastcall 
#else
#ifdef _MSC_VER
#define NO_INLINE __fastcall 
#endif
#endif

namespace NCompress {
namespace NBZip2 {

typedef NCompress::NHuffman::CDecoder<kMaxHuffmanLen, kMaxAlphaSize> CHuffmanDecoder;

class CDecoder;

struct CState
{
  UInt32 *Counters;

  #ifdef COMPRESS_BZIP2_MT

  CDecoder *Decoder;
  NWindows::CThread Thread;
  bool m_OptimizeNumTables;

  NWindows::NSynchronization::CAutoResetEvent StreamWasFinishedEvent;
  NWindows::NSynchronization::CAutoResetEvent WaitingWasStartedEvent;

  // it's not member of this thread. We just need one event per thread
  NWindows::NSynchronization::CAutoResetEvent CanWriteEvent;

  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.

  HRes Create();
  void FinishStream();
  void ThreadFunc();

  #endif

  CState(): Counters(0) {}
  ~CState() { Free(); }
  bool Alloc();
  void Free();
};

class CDecoder :
  public ICompressCoder,
  #ifdef COMPRESS_BZIP2_MT
  public ICompressSetCoderMt,
  #endif
  public ICompressGetInStreamProcessedSize,
  public CMyUnknownImp
{
public:
  COutBuffer m_OutStream;
  Byte MtPad[1 << 8]; // It's pad for Multi-Threading. Must be >= Cache_Line_Size.
  NStream::NMSBF::CDecoder<CInBuffer> m_InStream;
  Byte m_Selectors[kNumSelectorsMax];
  CHuffmanDecoder m_HuffmanDecoders[kNumTablesMax];
private:

  UInt32 m_NumThreadsPrev;

  UInt32 ReadBits(int numBits);
  Byte ReadByte();
  bool ReadBit();
  UInt32 ReadCRC();
  HRESULT PrepareBlock(CState &state);
  HRESULT DecodeFile(bool &isBZ, ICompressProgressInfo *progress);
  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  class CDecoderFlusher
  {
    CDecoder *_decoder;
  public:
    bool NeedFlush;
    CDecoderFlusher(CDecoder *decoder): _decoder(decoder), NeedFlush(true) {}
    ~CDecoderFlusher() 
    { 
      if (NeedFlush)
        _decoder->Flush();
      _decoder->ReleaseStreams(); 
    }
  };

public:
  CBZip2CombinedCRC CombinedCRC;

  #ifdef COMPRESS_BZIP2_MT
  ICompressProgressInfo *Progress;
  CState *m_States;

  NWindows::NSynchronization::CManualResetEvent CanProcessEvent;
  NWindows::NSynchronization::CCriticalSection CS;
  UInt32 NumThreads;
  bool MtMode;
  UInt32 NextBlockIndex;
  bool CloseThreads;
  bool StreamWasFinished1;
  bool StreamWasFinished2;
  NWindows::NSynchronization::CManualResetEvent CanStartWaitingEvent;

  HRESULT Result1;
  HRESULT Result2;

  UInt32 BlockSizeMax;
  CDecoder();
  ~CDecoder();
  HRes Create();
  void Free();

  #else
  CState m_States[1];
  #endif

  HRESULT ReadSignatures(bool &wasFinished, UInt32 &crc);


  HRESULT Flush() { return m_OutStream.Flush(); }
  void ReleaseStreams()
  {
    m_InStream.ReleaseStream();
    m_OutStream.ReleaseStream();
  }

  #ifdef COMPRESS_BZIP2_MT
  MY_UNKNOWN_IMP2(ICompressSetCoderMt, ICompressGetInStreamProcessedSize)
  #else
  MY_UNKNOWN_IMP1(ICompressGetInStreamProcessedSize)
  #endif

  
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);

  #ifdef COMPRESS_BZIP2_MT
  STDMETHOD(SetNumberOfThreads)(UInt32 numThreads);
  #endif
};

}}

#endif
