// DeflateDecoder.h

#pragma once

#ifndef __DEFLATE_DECODER_H
#define __DEFLATE_DECODER_H

#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/LSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../LZ/LZOutWindow.h"
#include "../Huffman/HuffmanDecoder.h"

#include "DeflateExtConst.h"

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
typedef NCompress::NHuffman::CDecoder<kNumHuffmanBits> CHuffmanDecoder;

class CCoder
{
  CLZOutWindow m_OutWindowStream;
  CInBit m_InBitStream;
  CHuffmanDecoder m_MainDecoder;
  CHuffmanDecoder m_DistDecoder;
  CHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  bool m_FinalBlock;
  bool m_StoredMode;
  UINT32 m_StoredBlockSize;

  bool _deflate64Mode;

  void DeCodeLevelTable(BYTE *newLevels, int numLevels);
  void ReadTables();
  
  /*
  void CCoder::ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *coder): m_Coder(coder) {}
    ~CCoderReleaser()
    {
      m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;
  */

public:
  CCoder(bool deflate64Mode = false);

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  HRESULT BaseGetInStreamProcessedSize(UINT64 *aValue);
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
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue);

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
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue);

  CCOMCoder64(): CCoder(true) {}
};

}}}

#endif