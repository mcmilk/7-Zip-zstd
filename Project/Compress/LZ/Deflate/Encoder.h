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
  void Init(UINT16 *aMatchDistances)
  {
    MatchDistances = aMatchDistances;
  };
};

struct COptimal
{
  UINT32 Price;
  UINT16 PosPrev;
  UINT16 BackPrev;
};

const kNumOpts = 0x1000;

class CCoder :
  public ICompressCoder,
  // public IInitMatchFinder,
  public ICompressSetEncoderProperties2,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressDeflateEncoder>
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

  BYTE m_LastLevels[kMaxTableSize];

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
  
  BYTE  m_LenPrices[kNumLenCombinations];
  BYTE  m_PosPrices[kDistTableSize];

  UINT32 m_CurrentBlockUncompressedSize;

  COnePosMatches *m_OnePosMatchesArray;
  UINT16 *m_OnePosMatchesMemory;

  UINT64 m_BlockStartPostion;
  int m_NumPasses;

  bool m_Created;

  HRESULT Create();
  void Free();

  void GetBacks(UINT32 aPos);

  void ReadGoodBacks();
  void MovePos(UINT32 aNum);
  UINT32 Backward(UINT32 &aBackRes, UINT32 aCur);
  UINT32 GetOptimal(UINT32 &aBackRes);

  void InitStructures();
  void CodeLevelTable(BYTE *aNewLevels, int aNumLevels, bool aCodeMode);
  int WriteTables(bool aWriteMode, bool anFinalBlock);
  void CopyBackBlockOp(UINT32 aDistance, UINT32 aLength);
  void WriteBlockData(bool aWriteMode, bool anFinalBlock);

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
  CCoder();
  ~CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    // COM_INTERFACE_ENTRY(IInitMatchFinder)
    COM_INTERFACE_ENTRY(ICompressSetEncoderProperties2)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()
  DECLARE_REGISTRY(CEncoder, TEXT("Compress.DeflateEncoder.1"), 
  TEXT("Compress.DeflateEncoder"), UINT(0), THREADFLAGS_APARTMENT)

  HRESULT CodeReal(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // IInitMatchFinder interface
  // STDMETHOD(InitMatchFinder)(IInWindowStreamMatch *aMatchFinder);

  // ICompressSetEncoderProperties2
  STDMETHOD(SetEncoderProperties2)(const PROPID *aPropIDs, 
      const PROPVARIANT *aProperties, UINT32 aNumProperties);
};

}}

#endif