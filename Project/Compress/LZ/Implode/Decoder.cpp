// Implode/Decoder.cpp

#include "StdAfx.h"

#include "Decoder.h"
#include "Common/Defs.h"

#include "Windows/Defs.h"

namespace NImplode {
namespace NDecoder {

/*
static const kImplodeHuffmanMaxValue = (1 << kBitBufferInValueNumValueBits);

static const kImplodeHuffmanValueMask = ((1 << kHuffmanMaxBits) - 1) << 
  (kBitBufferInValueNumValueBits - kHuffmanMaxBits);

// Implode consts
*/

static const int kNumDistanceLowDirectBitsForBigDict = 7;  
static const int kNumDistanceLowDirectBitsForSmallDict = 6;  

static const int kNumBitsInByte = 8;

static const int kLevelStructuresNumberFieldSize = kNumBitsInByte;
static const int kLevelStructuresNumberAdditionalValue = 1;

static const int kNumLevelStructureLevelBits = 4;
static const int kLevelStructureLevelAdditionalValue = 1;

static const int kNumLevelStructureRepNumberBits = 4;
static const int kLevelStructureRepNumberAdditionalValue = 1;


static const int kLiteralTableSize = (1 << kNumBitsInByte);
static const int kDistanceTableSize = 64;
static const int kLengthTableSize = 64;

static const UINT32 kHistorySize = 
    (1 << MyMax(kNumDistanceLowDirectBitsForBigDict, 
                kNumDistanceLowDirectBitsForSmallDict)) * 
    kDistanceTableSize; // = 8 KB;

static const int kNumAdditionalLengthBits = 8;

static const UINT32 kMatchMinLenWhenLiteralsOn = 3;  
static const UINT32 kMatchMinLenWhenLiteralsOff = 2;

static const UINT32 kMatchMinLenMax = MyMax(kMatchMinLenWhenLiteralsOn,
    kMatchMinLenWhenLiteralsOff);  // 3

static const UINT32 kMatchMaxLenMax = kMatchMinLenMax + 
    (kLengthTableSize - 1) + (1 << kNumAdditionalLengthBits) - 1;  // or 2

enum
{
  kMatchId = 0,
  kLiteralId = 1
};


/*

TTImplodeHuffmanTableDe::TTImplodeHuffmanTableDe(TTBitBufferInValue &aData, 
    UINT32 aNumSymbols):
  m_InBitStream(aData),
  FNumSymbols(aNumSymbols)
{
  FSymbols = new UINT32[FNumSymbols];
}

TTImplodeHuffmanTableDe::~TTImplodeHuffmanTableDe()
{
  delete []FSymbols;
}

class TTZipImplodeHuhmanException: public TTException {};

void TTImplodeHuffmanTableDe::MakeTable(const BYTE *aLevels)
{
  int aLenCounts[kNumImplodeHuffmanLevels + 1], aTmpPositions[kNumImplodeHuffmanLevels];
  int i;
  for(i = 0; i < kNumImplodeHuffmanLevels; i++)
    aLenCounts[i] = 0;

  UINT32 dw;
  for (dw = 0; dw < FNumSymbols; dw++)
    aLenCounts[aLevels[dw]]++;
  
  UINT32 aStartPos = 0;
  
  FLimits[kHuffmanMaxBits + 1] = 0;
  FPositions[kHuffmanMaxBits + 1] = 0;
  aLenCounts[kHuffmanMaxBits + 1] = 0;

  for (i = kHuffmanMaxBits; i > 0; i--)
  {
    aStartPos += aLenCounts[i] << (kBitBufferInValueNumValueBits - i);
    if (aStartPos > kImplodeHuffmanMaxValue)
      throw CDecoderException();
    FLimits[i] = aStartPos; 
    FPositions[i] = FPositions[i + 1] + aLenCounts[i + 1];
    aTmpPositions[i] = FPositions[i] + aLenCounts[i];
  }
  if (aStartPos != kImplodeHuffmanMaxValue)
    throw TTZipImplodeHuhmanException();
  
  for (dw = 0; dw < FNumSymbols; dw++)
    if (aLevels[dw] != 0)
      FSymbols[--aTmpPositions[aLevels[dw]]] = dw;
}

UINT32 TTImplodeHuffmanTableDe::DecodeSymbol()
{
  UINT32 aNumBits;
  UINT32 aValue = m_InBitStream.GetValue() & kImplodeHuffmanValueMask;
  int i;
  for(i = kHuffmanMaxBits; i > 0; i--)
  {
    if (aValue < FLimits[i])
    {
      aNumBits = i;
      break;
    }
  }
  if (i == 0)
    throw 1012204;
 
  m_InBitStream.MovePos(aNumBits);
  UINT32 anIndex = FPositions[aNumBits] + ((aValue - FLimits[aNumBits + 1]) 
    >> (kBitBufferInValueNumValueBits - aNumBits));
  if (anIndex >= FNumSymbols)
    throw 1012332; // test it
  return FSymbols[anIndex];
}
*/

// ---------------------------


CCoder::CCoder():
  m_LiteralDecoder(kLiteralTableSize),
  m_LengthDecoder(kLengthTableSize),
  m_DistanceDecoder(kDistanceTableSize)
{
  m_OutWindowStream.Create(kHistorySize/*, kMatchMaxLenMax*/);
}

CCoder::~CCoder()
{}

STDMETHODIMP CCoder::Init(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream)
{
  m_OutWindowStream.Init(anOutStream, false);
  m_InBitStream.Init(anInStream);
  return S_OK;
}

STDMETHODIMP CCoder::ReleaseStreams()
{
  m_OutWindowStream.ReleaseStream();
  m_InBitStream.ReleaseStream();
  return S_OK;
}

STDMETHODIMP CCoder::Flush()
{
  m_OutWindowStream.Flush();
  return S_OK;
}

void CCoder::ReadLevelItems(NImplode::NHuffman::CDecoder &aDecoder, 
    BYTE *aLevels, int aNumLevelItems)
{
  int aNumCodedStructures = m_InBitStream.ReadBits(kNumBitsInByte) + 
      kLevelStructuresNumberAdditionalValue;
  int aCurrentIndex = 0;
  for(int i = 0; i < aNumCodedStructures; i++)
  {
    int aLevel = m_InBitStream.ReadBits(kNumLevelStructureLevelBits) + 
      kLevelStructureLevelAdditionalValue;
    int aRep = m_InBitStream.ReadBits(kNumLevelStructureRepNumberBits) + 
      kLevelStructureRepNumberAdditionalValue;
    if (aCurrentIndex + aRep > aNumLevelItems)
      throw CException(CException::kData);
    for(int j = 0; j < aRep; j++)
      aLevels[aCurrentIndex++] = aLevel;
  }
  if (aCurrentIndex != aNumLevelItems)
    throw CException(CException::kData);
  try
  {
    aDecoder.SetCodeLengths(aLevels);
  }
  catch(const NImplode::NHuffman::CDecoderException &)
  {
    throw CException(CException::kData);
  }
}


void CCoder::ReadTables(void)
{
  if (m_LiteralsOn)
  {
    BYTE aLiteralLevels[kLiteralTableSize];
    ReadLevelItems(m_LiteralDecoder, aLiteralLevels, kLiteralTableSize);
  }

  BYTE aLengthLevels[kLengthTableSize];
  ReadLevelItems(m_LengthDecoder, aLengthLevels, kLengthTableSize);

  BYTE aDistanceLevels[kDistanceTableSize];
  ReadLevelItems(m_DistanceDecoder, aDistanceLevels, kDistanceTableSize);

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

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;
  UINT64 aPos = 0, aUnPackSize = *anOutSize;

  Init(anInStream, anOutStream);
  CCoderReleaser aCoderReleaser(this);

  ReadTables();
  
  //  FStat.Update(kAllBytes, aNext - m_Position);
  while(aPos < aUnPackSize)
  {
    if (aProgress != NULL && aPos % (1 << 16) == 0)
    {
      UINT64 aPackSize = m_InBitStream.GetProcessedSize();
      HRESULT aResult = aProgress->SetRatioInfo(&aPackSize, &aPos);
      if (aResult != S_OK)
        return aResult;
    }
    if(m_InBitStream.ReadBits(1) == kMatchId) // match
    {
      UINT32 aLowDistBits = m_InBitStream.ReadBits(m_NumDistanceLowDirectBits);
      UINT32 aDistance = (m_DistanceDecoder.DecodeSymbol(&m_InBitStream) << 
          m_NumDistanceLowDirectBits) + aLowDistBits;

      UINT32 aLengthSymbol = m_LengthDecoder.DecodeSymbol(&m_InBitStream);
      UINT32 aLength = aLengthSymbol + m_MinMatchLength;
      if (aLengthSymbol == kLengthTableSize - 1) // special symbol  = 63
        aLength += m_InBitStream.ReadBits(kNumAdditionalLengthBits);
      /*
      if (m_Position + aLength > FOutputLength) 
        throw TTZipImplodeDeCodeException();
      */
      while(aDistance >= aPos && aLength > 0)
      {
        m_OutWindowStream.PutOneByte(0);
        aPos++;
        aLength--;
      }
      if (aLength > 0)
        m_OutWindowStream.CopyBackBlock(aDistance, aLength);
      aPos += aLength;
    }
    else
    {
      BYTE aChar = BYTE(m_LiteralsOn ? m_LiteralDecoder.DecodeSymbol(&m_InBitStream) : 
          m_InBitStream.ReadBits(kNumBitsInByte));
      m_OutWindowStream.PutOneByte(aChar);
      aPos++;
    }
  }
  if (aPos > aUnPackSize)
    throw CException(CException::kData);
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(const NStream::NWindow::COutWriteException &outWriteException)
  {
    return outWriteException.Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}

STDMETHODIMP CCoder::SetDecoderProperties(ISequentialInStream *anInStream)
{
  BYTE aBigDictionaryOn, aLiteralsOn;

  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(anInStream->Read(&aBigDictionaryOn,
      sizeof(aBigDictionaryOn), &aProcessedSize));
  if (aProcessedSize != sizeof(aBigDictionaryOn))
    return E_INVALIDARG;

  RETURN_IF_NOT_S_OK(anInStream->Read(&aLiteralsOn,
      sizeof(aLiteralsOn), &aProcessedSize));
  if (aProcessedSize != sizeof(aLiteralsOn))
    return E_INVALIDARG;

  m_BigDictionaryOn = aBigDictionaryOn != 0 ? true : false;

  m_NumDistanceLowDirectBits = m_BigDictionaryOn ? kNumDistanceLowDirectBitsForBigDict:
    kNumDistanceLowDirectBitsForSmallDict;

  m_LiteralsOn = aLiteralsOn != 0 ? true : false;
 
  m_MinMatchLength = m_LiteralsOn ? kMatchMinLenWhenLiteralsOn : 
    kMatchMinLenWhenLiteralsOff;

  return S_OK;
}

}}
