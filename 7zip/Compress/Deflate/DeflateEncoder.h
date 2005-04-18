// DeflateEncoder.h

#ifndef __DEFLATE_ENCODER_H
#define __DEFLATE_ENCODER_H

#include "Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/LSBFEncoder.h"
#include "../LZ/IMatchFinder.h"
#include "../Huffman/HuffmanEncoder.h"

#include "DeflateConst.h"

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

struct CCodeValue
{
  Byte Flag;
  union
  {
    Byte Imm;
    Byte Len;
  };
  UInt16 Pos;
};

class COnePosMatches
{
public:
  UInt16 *MatchDistances;
  UInt16 LongestMatchLength;    
  UInt16 LongestMatchDistance;
  void Init(UInt16 *matchDistances)
  {
    MatchDistances = matchDistances;
  };
};

struct COptimal
{
  UInt32 Price;
  UInt16 PosPrev;
  UInt16 BackPrev;
};

const int kNumOpts = 0x1000;

class CCoder
{
  UInt32 m_FinderPos;
  
  COptimal m_Optimum[kNumOpts];
  
  CMyComPtr<IMatchFinder> m_MatchFinder;

  NStream::NLSBF::CEncoder m_OutStream;
  NStream::NLSBF::CReverseEncoder m_ReverseOutStream;
  
  NCompression::NHuffman::CEncoder m_MainCoder;
  NCompression::NHuffman::CEncoder m_DistCoder;
  NCompression::NHuffman::CEncoder m_LevelCoder;

  Byte m_LastLevels[kMaxTableSize64];

  UInt32 m_ValueIndex;
  CCodeValue *m_Values;

  UInt32 m_OptimumEndIndex;
  UInt32 m_OptimumCurrentIndex;
  UInt32 m_AdditionalOffset;

  UInt32 m_LongestMatchLength;    
  UInt32 m_LongestMatchDistance;
  UInt16 *m_MatchDistances;

  UInt32 m_NumFastBytes;
  UInt32 m_MatchLengthEdge;

  Byte  m_LiteralPrices[256];
  
  Byte  m_LenPrices[kNumLenCombinations32];
  Byte  m_PosPrices[kDistTableSize64];

  UInt32 m_CurrentBlockUncompressedSize;

  COnePosMatches *m_OnePosMatchesArray;
  UInt16 *m_OnePosMatchesMemory;

  UInt64 m_BlockStartPostion;
  int m_NumPasses;

  bool m_Created;

  bool _deflate64Mode;
  UInt32 m_NumLenCombinations;
  UInt32 m_MatchMaxLen;
  const Byte *m_LenStart;
  const Byte *m_LenDirectBits;

  HRESULT Create();
  void Free();

  void GetBacks(UInt32 aPos);

  void ReadGoodBacks();
  void MovePos(UInt32 num);
  UInt32 Backward(UInt32 &backRes, UInt32 cur);
  UInt32 GetOptimal(UInt32 &backRes);

  void InitStructures();
  void CodeLevelTable(Byte *newLevels, int numLevels, bool codeMode);
  int WriteTables(bool writeMode, bool finalBlock);
  void CopyBackBlockOp(UInt32 distance, UInt32 length);
  void WriteBlockData(bool writeMode, bool finalBlock);

  void CCoder::ReleaseStreams()
  {
    // m_MatchFinder.ReleaseStream();
    m_OutStream.ReleaseStream();
  }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *coder): m_Coder(coder) {}
    ~CCoderReleaser() { m_Coder->ReleaseStreams(); }
  };
  friend class CCoderReleaser;

public:
  CCoder(bool deflate64Mode = false);
  ~CCoder();

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetCoderProperties
  HRESULT BaseSetEncoderProperties2(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
};

///////////////////////////////////////////////////////////////

class CCOMCoder :
  public ICompressCoder,
  public ICompressSetCoderProperties, 
  public CMyUnknownImp,
  public CCoder
{
public:
  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
  CCOMCoder(): CCoder(false) {};
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  // ICompressSetCoderProperties
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
};

class CCOMCoder64 :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp,
  public CCoder
{
public:
  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)
  CCOMCoder64(): CCoder(true) {};
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  // ICompressSetCoderProperties
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
};


}}}

#endif
