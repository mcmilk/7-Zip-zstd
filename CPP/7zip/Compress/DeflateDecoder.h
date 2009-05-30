// DeflateDecoder.h

#ifndef __DEFLATE_DECODER_H
#define __DEFLATE_DECODER_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitlDecoder.h"
#include "DeflateConst.h"
#include "HuffmanDecoder.h"
#include "LzOutWindow.h"

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
  CLzOutWindow m_OutWindowStream;
  NBitl::CDecoder<CInBuffer> m_InBitStream;
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
  bool _needInitInStream;
  Int32 _remainLen;
  UInt32 _rep0;
  bool _needReadTable;

  UInt32 ReadBits(int numBits);

  bool DeCodeLevelTable(Byte *values, int numSymbols);
  bool ReadTables();
  
  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  class CCoderReleaser
  {
    CCoder *_coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): _coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        _coder->Flush();
      _coder->ReleaseOutStream();
    }
  };
  friend class CCoderReleaser;

  HRESULT CodeSpec(UInt32 curSize);
public:
  bool ZlibMode;
  Byte ZlibFooter[4];

  CCoder(bool deflate64Mode, bool deflateNSIS = false);
  virtual ~CCoder() {};

  void SetKeepHistory(bool keepHistory) { _keepHistory = keepHistory; }

  void ReleaseOutStream()
  {
    m_OutWindowStream.ReleaseStream();
  }

  HRESULT CodeReal(ISequentialOutStream *outStream,
      const UInt64 *outSize, ICompressProgressInfo *progress);

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

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(SetInStream)(ISequentialInStream *inStream);
  STDMETHOD(ReleaseInStream)();
  STDMETHOD(SetOutStreamSize)(const UInt64 *outSize);
  
  #ifndef NO_READ_FROM_CODER
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  #endif

  STDMETHOD(CodeResume)(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress);

  HRESULT InitInStream(bool needInit)
  {
    if (!m_InBitStream.Create(1 << 17))
      return E_OUTOFMEMORY;
    if (needInit)
    {
      m_InBitStream.Init();
      _needInitInStream = false;
    }
    return S_OK;
  }

  void AlignToByte() { m_InBitStream.AlignToByte(); }
  Byte ReadByte() { return (Byte)m_InBitStream.ReadBits(8); }
  bool InputEofError() const { return m_InBitStream.ExtraBitsWereRead(); }
  UInt64 GetInputProcessedSize() const { return m_InBitStream.GetProcessedSize(); }

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
