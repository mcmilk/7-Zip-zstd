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

class CCoder
{
  CLZOutWindow m_OutWindowStream;
  CInBit m_InBitStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kStaticMainTableSize> m_MainDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kStaticDistTableSize> m_DistDecoder;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  bool m_FinalBlock;
  bool m_StoredMode;
  UInt32 m_StoredBlockSize;

  bool _deflate64Mode;

  void DeCodeLevelTable(Byte *newLevels, int numLevels);
  void ReadTables();
  
  void CCoder::ReleaseStreams()
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

public:
  CCoder(bool deflate64Mode = false);

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  HRESULT BaseGetInStreamProcessedSize(UInt64 *aValue);
};

class CCOMCoder :
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  public CMyUnknownImp,
  public CCoder
{
public:

  MY_UNKNOWN_IMP1(ICompressGetInStreamProcessedSize)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UInt64 *aValue);

  CCOMCoder(): CCoder(false) {}
};

class CCOMCoder64 :
  public ICompressCoder,
  public ICompressGetInStreamProcessedSize,
  public CMyUnknownImp,
  public CCoder
{
public:
  MY_UNKNOWN_IMP1(ICompressGetInStreamProcessedSize)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UInt64 *aValue);

  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif
