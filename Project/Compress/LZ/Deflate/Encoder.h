// Deflate/Encoder.h

#pragma once

#ifndef __DEFLATE_ENCODER_H
#define __DEFLATE_ENCODER_H

#include "../../Interface/CompressInterface.h"
#include "../MatchFinder/BinTree/BinTree3Z.h"
#include "Stream/LSBFEncoder.h"

#include "Compression/HuffmanEncoder.h"

#include "Const.h"

// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0401-090000000100}
DEFINE_GUID(CLSID_CCompressDeflate64Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00);

namespace NDeflate {
namespace NEncoder {

struct CCodeValue
{
  BYTE Flag;
  union
  {
    BYTE Imm;
    BYTE Len;
  };
  UINT16 Pos;
};

class COnePosMatches
{
public:
  UINT16 *MatchDistances;
  UINT16 LongestMatchLength;    
  UINT16 LongestMatchDistance;
  void Init(UINT16 *matchDistances)
  {
    MatchDistances = matchDistances;
  };
};

struct COptimal
{
  UINT32 Price;
  UINT16 PosPrev;
  UINT16 BackPrev;
};

const kNumOpts = 0x1000;

class CCoder
{
  UINT32 m_FinderPos;
  
  COptimal m_Optimum[kNumOpts];
  
  // CComPtr<IInWindowStreamMatch> m_MatchFinder;
  NBT3Z::CInTree m_MatchFinder;

  NStream::NLSBF::CEncoder m_OutStream;
  NStream::NLSBF::CReverseEncoder m_ReverseOutStream;
  
  NCompression::NHuffman::CEncoder m_MainCoder;
  NCompression::NHuffman::CEncoder m_DistCoder;
  NCompression::NHuffman::CEncoder m_LevelCoder;

  BYTE m_LastLevels[kMaxTableSize64];

  UINT32 m_ValueIndex;
  CCodeValue *m_Values;

  UINT32 m_OptimumEndIndex;
  UINT32 m_OptimumCurrentIndex;
  UINT32 m_AdditionalOffset;

  UINT32 m_LongestMatchLength;    
  UINT32 m_LongestMatchDistance;
  UINT16 *m_MatchDistances;

  UINT32 m_NumFastBytes;
  UINT32 m_MatchLengthEdge;

  BYTE  m_LiteralPrices[256];
  
  BYTE  m_LenPrices[kNumLenCombinations32];
  BYTE  m_PosPrices[kDistTableSize64];

  UINT32 m_CurrentBlockUncompressedSize;

  COnePosMatches *m_OnePosMatchesArray;
  UINT16 *m_OnePosMatchesMemory;

  UINT64 m_BlockStartPostion;
  int m_NumPasses;

  bool m_Created;

  bool _deflate64Mode;
  UINT32 m_NumLenCombinations;
  UINT32 m_MatchMaxLen;
  const BYTE *m_LenStart;
  const BYTE *m_LenDirectBits;

  HRESULT Create();
  void Free();

  void GetBacks(UINT32 aPos);

  void ReadGoodBacks();
  void MovePos(UINT32 num);
  UINT32 Backward(UINT32 &backRes, UINT32 cur);
  UINT32 GetOptimal(UINT32 &backRes);

  void InitStructures();
  void CodeLevelTable(BYTE *newLevels, int numLevels, bool codeMode);
  int WriteTables(bool writeMode, bool finalBlock);
  void CopyBackBlockOp(UINT32 distance, UINT32 length);
  void WriteBlockData(bool writeMode, bool finalBlock);

  void CCoder::ReleaseStreams()
  {
    m_MatchFinder.ReleaseStream();
    m_OutStream.ReleaseStream();
  }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  CCoder(bool deflate64Mode = false);
  ~CCoder();

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetEncoderProperties2
  HRESULT BaseSetEncoderProperties2(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);
};

///////////////////////////////////////////////////////////////

class CCOMCoder :
  public ICompressCoder,
  // public IInitMatchFinder,
  public ICompressSetEncoderProperties2,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressDeflateEncoder>,
  public CCoder
{
public:
  CCOMCoder(): CCoder(false) {};
  BEGIN_COM_MAP(CCOMCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(ICompressSetEncoderProperties2)
  END_COM_MAP()
  DECLARE_NOT_AGGREGATABLE(CCOMCoder)
  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CCOMCoder, 
    // TEXT("Compress.DeflateEncoder.1"), TEXT("Compress.DeflateEncoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
  // ICompressSetEncoderProperties2
  STDMETHOD(SetEncoderProperties2)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);
};

class CCOMCoder64 :
  public ICompressCoder,
  // public IInitMatchFinder,
  public ICompressSetEncoderProperties2,
  public CComObjectRoot,
  public CComCoClass<CCOMCoder64, &CLSID_CCompressDeflate64Encoder>,
  public CCoder
{
public:
  CCOMCoder64(): CCoder(true) {};
  BEGIN_COM_MAP(CCOMCoder64)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(ICompressSetEncoderProperties2)
  END_COM_MAP()
  DECLARE_NOT_AGGREGATABLE(CCOMCoder64)
  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CCOMCoder64, 
    // TEXT("Compress.DeflateEncoder.1"), TEXT("Compress.DeflateEncoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
  // ICompressSetEncoderProperties2
  STDMETHOD(SetEncoderProperties2)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UINT32 numProperties);
};


}}

#endif