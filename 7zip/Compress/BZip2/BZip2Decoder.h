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

namespace NCompress {
namespace NBZip2 {

typedef NCompress::NHuffman::CDecoder<kMaxHuffmanLen, kMaxAlphaSize> CHuffmanDecoder;

struct CState
{
  UInt32 *tt;
  bool BlockRandomised;
  UInt32 OrigPtr;
  UInt32 BlockSize;
  UInt32 CharCounters[256];
  Byte m_Selectors[kNumSelectorsMax];
  UInt32 StoredCRC;

  CState(): tt(0) {}
  ~CState();
  bool Alloc();
  HRESULT DecodeBlock(COutBuffer &m_OutStream);
};

class CDecoder :
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  public CMyUnknownImp
{
  NStream::NMSBF::CDecoder<CInBuffer> m_InStream;
  COutBuffer m_OutStream;

  CHuffmanDecoder m_HuffmanDecoders[kNumTablesMax];
  
  CState m_State;

  UInt32 ReadBits(int numBits);
  Byte ReadByte();
  bool ReadBit();
  UInt32 ReadCRC();
  HRESULT ReadBlock(UInt32 blockSizeMax, CState &state);
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
  HRESULT Flush() { return m_OutStream.Flush(); }
  void ReleaseStreams()
  {
    m_InStream.ReleaseStream();
    m_OutStream.ReleaseStream();
  }

  MY_UNKNOWN_IMP1(ICompressGetInStreamProcessedSize)
  
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);
};

}}

#endif
