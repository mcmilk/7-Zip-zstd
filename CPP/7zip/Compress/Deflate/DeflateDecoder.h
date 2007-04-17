// DeflateDecoder.h

#ifndef __DEFLATE_DECODER_H
#define __DEFLATE_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/LSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../LZ/LZOutWindow.h"
#include "../Huffman/HuffmanDecoder.h"

#include "DeflateConst.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

class CCoder:
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  #ifndef NO_READ_FROM_CODER
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
  #endif
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  NStream::NLSBF::CDecoder<CInBuffer> m_InBitStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kFixedMainTableSize> m_MainDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kFixedDistTableSize> m_DistDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  UInt32 m_StoredBlockSize;

  bool m_FinalBlock;
  bool m_StoredMode;
  UInt32 _numDistLevels;


  bool _deflateNSIS;
  bool _deflate64Mode;
  bool _keepHistory;
  Int32 _remainLen;
  UInt32 _rep0;
  bool _needReadTable;

  UInt32 ReadBits(int numBits);

  bool DeCodeLevelTable(Byte *values, int numSymbols);
  bool ReadTables();
  
  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    ReleaseInStream();
  }

  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): m_Coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        m_Coder->Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

  HRESULT CodeSpec(UInt32 curSize);
public:
  CCoder(bool deflate64Mode, bool deflateNSIS = false);
  void SetKeepHistory(bool keepHistory) { _keepHistory = keepHistory; }

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  #ifndef NO_READ_FROM_CODER
  MY_UNKNOWN_IMP4(
      ICompressGetInStreamProcessedSize,
      ICompressSetInStream, 
      ICompressSetOutStreamSize,
      ISequentialInStream
      )
  #else
  MY_UNKNOWN_IMP1(
      ICompressGetInStreamProcessedSize)
  #endif

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);
  
  #ifndef NO_READ_FROM_CODER
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);
};

class CCOMCoder : public CCoder
{
public:
  CCOMCoder(): CCoder(false) {}
};

class CNsisCOMCoder : public CCoder
{
public:
  CNsisCOMCoder(): CCoder(false, true) {}
};

class CCOMCoder64 : public CCoder
{
public:
  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif
