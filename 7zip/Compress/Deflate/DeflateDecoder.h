// DeflateDecoder.h

#ifndef __DEFLATE_DECODER_H
#define __DEFLATE_DECODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/LSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../LZ/LZOutWindow.h"
#include "../Huffman/HuffmanDecoder.h"

#include "DeflateExtConst.h"
#include "DeflateConst.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

class CException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CException(ECauseType aCause): m_Cause(aCause) {}
};

typedef NStream::NLSBF::CDecoder<CInBuffer> CInBit;

class CCoder:
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  #ifdef _ST_MODE
  public ICompressSetInStream,
  public ICompressSetOutStreamSize,
  public ISequentialInStream,
  #endif
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  CInBit m_InBitStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kStaticMainTableSize> m_MainDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kStaticDistTableSize> m_DistDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  UInt32 m_StoredBlockSize;

  bool m_FinalBlock;
  bool m_StoredMode;
  bool _deflate64Mode;
  bool _keepHistory;
  int _remainLen;
  UInt32 _rep0;
  bool _needReadTable;


  void DeCodeLevelTable(Byte *newLevels, int numLevels);
  bool ReadTables();
  
  void CCoder::ReleaseStreams()
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
  CCoder(bool deflate64Mode);
  void SetKeepHistory(bool keepHistory) { _keepHistory = keepHistory; }

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  #ifdef _ST_MODE
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
  
  #ifdef _ST_MODE
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UInt64 *value);
};

class CCOMCoder :
  public CCoder
{
public:
  CCOMCoder(): CCoder(false) {}
};

class CCOMCoder64 :
  public CCoder
{
public:
  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif
