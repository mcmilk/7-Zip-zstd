// LzhDecoder.h

#ifndef __COMPRESS_LZH_DECODER_H
#define __COMPRESS_LZH_DECODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../../Common/MSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../Huffman/HuffmanDecoder.h"
#include "../LZ/LZOutWindow.h"

namespace NCompress {
namespace NLzh {
namespace NDecoder {

const int kMaxHuffmanLen = 16; // Check it

const int kNumSpecLevelSymbols = 3;
const int kNumLevelSymbols = kNumSpecLevelSymbols + kMaxHuffmanLen;

const int kDictBitsMax = 16;
const int kNumDistanceSymbols = kDictBitsMax + 1;

const int kMaxMatch = 256;
const int kMinMatch = 3;
const int kNumCSymbols = 256 + kMaxMatch + 2 - kMinMatch;

template <UInt32 m_NumSymbols>
class CHuffmanDecoder:public NCompress::NHuffman::CDecoder<kMaxHuffmanLen, m_NumSymbols>
{
public:
  int Symbol;
  template <class TBitDecoder>
  UInt32 Decode(TBitDecoder *bitStream)
  {
    if (Symbol >= 0)
      return (UInt32)Symbol;
    return DecodeSymbol(bitStream);
  }
};

class CCoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  NStream::NMSBF::CDecoder<CInBuffer> m_InBitStream;

  int m_NumDictBits;

  CHuffmanDecoder<kNumLevelSymbols> m_LevelHuffman;
  CHuffmanDecoder<kNumDistanceSymbols> m_PHuffmanDecoder;
  CHuffmanDecoder<kNumCSymbols> m_CHuffmanDecoder;

  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }

  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): m_Coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

  void MakeTable(int nchar, Byte *bitlen, int tablebits, 
      UInt32 *table, int tablesize);
  
  UInt32 ReadBits(int numBits);
  HRESULT ReadLevelTable();
  HRESULT ReadPTable(int numBits);
  HRESULT ReadCTable();

public:
  
  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  void SetDictionary(int numDictBits) { m_NumDictBits = numDictBits; }
  CCoder(): m_NumDictBits(0) {}
};

}}}

#endif
