// Deflate/Encoder.cpp

#include "StdAfx.h"

#include "Encoder.h"
#include "Const.h"

#include "Windows/Defs.h"
#include "Windows/COMTRY.h"
#include "../MatchFinder/BinTree/BinTree3Main.h"

namespace NDeflate {
namespace NEncoder {

class CMatchFinderException
{
public:
  HRESULT m_Result;
  CMatchFinderException(HRESULT aResult): m_Result (aResult) {}
};

static const kValueBlockSize = 0x2000;

static const kMaxCodeBitLength = 15;
static const kMaxLevelBitLength = 7;

static const BYTE kFlagImm     = 0;
static const BYTE kFlagLenPos  = 4;

static const UINT32 kMaxUncompressedBlockSize = 0xFFFF; // test it !!!

static const UINT32 kBlockUncompressedSizeThreshold = 
    kMaxUncompressedBlockSize - kMatchMaxLen - kNumOpts;

static const kNumGoodBacks = 0x10000; 

static BYTE kNoLiteralDummy = 13;
static BYTE kNoLenDummy = 13;
static BYTE kNoPosDummy = 6;

static BYTE g_LenSlots[kNumLenCombinations];
static BYTE g_FastPos[1 << 8];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    int i;
    for(i = 0; i < kLenTableSize; i++)
    {
      int c = kLenStart[i];
      int j = 1 << kLenDirectBits[i];
      for(int k = 0; k < j; k++, c++)
        g_LenSlots[c] = i;
    }
    
    const kFastSlots = 16;
    int c = 0;
    for (BYTE aSlotFast = 0; aSlotFast < kFastSlots; aSlotFast++)
    {
      UINT32 k = (1 << kDistDirectBits[aSlotFast]);
      for (UINT32 j = 0; j < k; j++, c++)
        g_FastPos[c] = aSlotFast;
    }
  }
};

static CFastPosInit g_FastPosInit;


inline UINT32 GetPosSlot(UINT32 aPos)
{
  //  for (UINT32 i = 1; aPos >= kDistStart[i]; i++);
  //    return i - 1;
  if (aPos < 0x100)
    return g_FastPos[aPos];
  return g_FastPos[aPos >> 7] + 14;
}

CCoder::CCoder():
  m_MainCoder(kMainTableSize, kLenDirectBits, kMatchNumber, kMaxCodeBitLength),
  m_DistCoder(kDistTableSize, kDistDirectBits, 0, kMaxCodeBitLength),
  m_LevelCoder(kLevelTableSize, kLevelDirectBits, 0, kMaxLevelBitLength),
  m_NumPasses(1),
  m_NumFastBytes(32),
  m_OnePosMatchesMemory(0),
  m_OnePosMatchesArray(0),
  m_MatchDistances(0),
  m_Created(false),
  m_Values(0)
{
  m_Values = new CCodeValue[kValueBlockSize + kNumOpts];
}

HRESULT CCoder::Create()
{
  COM_TRY_BEGIN
  m_MatchFinder.Create(kHistorySize, kNumOpts + kNumGoodBacks, m_NumFastBytes, 
      kMatchMaxLen - m_NumFastBytes);
  m_MatchLengthEdge = m_NumFastBytes + 1;

  if (m_NumPasses > 1)
  {
    m_OnePosMatchesMemory = new UINT16[kNumGoodBacks * (m_NumFastBytes + 1)];
    try
    {
      m_OnePosMatchesArray = new COnePosMatches[kNumGoodBacks];
    }
    catch(...)
    {
      delete []m_OnePosMatchesMemory;
      m_OnePosMatchesMemory = 0;
      throw;
    }
    UINT16 *aGoodBacksWordsCurrent = m_OnePosMatchesMemory;
    for(int i = 0; i < kNumGoodBacks; i++, aGoodBacksWordsCurrent += (m_NumFastBytes + 1))
      m_OnePosMatchesArray[i].Init(aGoodBacksWordsCurrent);
  }
  else
    m_MatchDistances = new UINT16[m_NumFastBytes + 1];
  return S_OK;
  COM_TRY_END
}

// ICompressSetEncoderProperties2
STDMETHODIMP CCoder::SetEncoderProperties2(const PROPID *aPropIDs, 
    const PROPVARIANT *aProperties, UINT32 aNumProperties)
{
  for(UINT32 i = 0; i < aNumProperties; i++)
  {
    const PROPVARIANT &aProperty = aProperties[i]; 
    switch(aPropIDs[i])
    {
      case NEncodingProperies::kNumPasses:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumPasses = aProperty.ulVal;
        if(m_NumPasses == 0 || m_NumPasses > 255)
          return E_INVALIDARG;
        break;
      case NEncodingProperies::kNumFastBytes:
        if (aProperty.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumFastBytes = aProperty.ulVal;
        if(m_NumFastBytes < 3 || m_NumFastBytes > kMatchMaxLen)
          return E_INVALIDARG;
        break;
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}
  
void CCoder::Free()
{
  if(m_NumPasses > 0)
  {
    if (m_NumPasses > 1)
    {
      delete []m_OnePosMatchesMemory;
      delete []m_OnePosMatchesArray;
    }
    else
      delete []m_MatchDistances;
  }
}

CCoder::~CCoder()
{
  Free();
  delete []m_Values;
}

void CCoder::ReadGoodBacks()
{
  UINT32 aGoodIndex;
  if (m_NumPasses > 1)
  {
    aGoodIndex = m_FinderPos % kNumGoodBacks;
    m_MatchDistances = m_OnePosMatchesArray[aGoodIndex].MatchDistances;
  }
  UINT32 aDistanceTmp[kMatchMaxLen + 1];
  UINT32 aLen = m_MatchFinder.GetLongestMatch(aDistanceTmp);
  for(UINT32 i = kMatchMinLen; i <= aLen; i++)
    m_MatchDistances[i] = aDistanceTmp[i];

  m_LongestMatchDistance = m_MatchDistances[aLen];
  if (aLen == m_NumFastBytes && m_NumFastBytes != kMatchMaxLen)
    m_LongestMatchLength = aLen + m_MatchFinder.GetMatchLen(aLen, 
        m_LongestMatchDistance, kMatchMaxLen - aLen);
  else
    m_LongestMatchLength = aLen;
  if (m_NumPasses > 1)
  {
    m_OnePosMatchesArray[aGoodIndex].LongestMatchDistance = UINT16(m_LongestMatchDistance);
    m_OnePosMatchesArray[aGoodIndex].LongestMatchLength = UINT16(m_LongestMatchLength);
  }
  HRESULT aResult = m_MatchFinder.MovePos();
  if (aResult != S_OK)
    throw CMatchFinderException(aResult);
  m_FinderPos++;
  m_AdditionalOffset++;
}

void CCoder::GetBacks(UINT32 aPos)
{
  if(aPos == m_FinderPos)
    ReadGoodBacks();
  else
  {
    if (m_NumPasses == 1)
    {
      if(aPos + 1 == m_FinderPos) 
        return;
      throw 1932;   
    }
    else
    {
      UINT32 aGoodIndex = aPos % kNumGoodBacks;
      m_MatchDistances = m_OnePosMatchesArray[aGoodIndex].MatchDistances;
      m_LongestMatchDistance = m_OnePosMatchesArray[aGoodIndex].LongestMatchDistance;
      m_LongestMatchLength = m_OnePosMatchesArray[aGoodIndex].LongestMatchLength;
    }
  }
}


void CCoder::MovePos(UINT32 aNum)
{
  if (m_NumPasses > 1)
  {
    for(UINT32 i = 0; i < aNum; i++)
      GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + i + 1));
  }
  else
  {
    for (;aNum > 0; aNum--)
    {
      m_MatchFinder.DummyLongestMatch();
      HRESULT aResult = m_MatchFinder.MovePos();
      if (aResult != S_OK)
        throw CMatchFinderException(aResult);
      m_FinderPos++;
      m_AdditionalOffset++;
    }
  }
}

static const kIfinityPrice = 0xFFFFFFF;

UINT32 CCoder::Backward(UINT32 &aBackRes, UINT32 aCur)
{
  m_OptimumEndIndex = aCur;
  UINT32 aPosMem = m_Optimum[aCur].PosPrev;
  UINT16 aBackMem = m_Optimum[aCur].BackPrev;
  do
  {
    UINT32 aPosPrev = aPosMem;
    UINT16 aBackCur = aBackMem;
    aBackMem = m_Optimum[aPosPrev].BackPrev;
    aPosMem = m_Optimum[aPosPrev].PosPrev;
    m_Optimum[aPosPrev].BackPrev = aBackCur;
    m_Optimum[aPosPrev].PosPrev = aCur;
    aCur = aPosPrev;
  }
  while(aCur > 0);
  aBackRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex  = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex; 
}

UINT32 CCoder::GetOptimal(UINT32 &aBackRes)
{
  if(m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    UINT32 aLen = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    aBackRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return aLen;
  }
  m_OptimumCurrentIndex = 0;
  m_OptimumEndIndex = 0;
  
  GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize));

  UINT32 aLenMain = m_LongestMatchLength;
  UINT32 aBackMain = m_LongestMatchDistance;

  if(aLenMain < kMatchMinLen)
    return 1;
  if(aLenMain >= m_MatchLengthEdge)
  {
    aBackRes = aBackMain; 
    MovePos(aLenMain - 1);
    return aLenMain;
  }
  m_Optimum[1].Price = m_LiteralPrices[m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset)];
  m_Optimum[1].PosPrev = 0;

  m_Optimum[2].Price = kIfinityPrice;
  m_Optimum[2].PosPrev = 1;

  for(UINT32 i = kMatchMinLen; i <= aLenMain; i++)
  {
    m_Optimum[i].PosPrev = 0;
    m_Optimum[i].BackPrev = m_MatchDistances[i];
    m_Optimum[i].Price = m_LenPrices[i - kMatchMinLen] + m_PosPrices[GetPosSlot(m_MatchDistances[i])];
  }


  UINT32 aCur = 0;
  UINT32 aLenEnd = aLenMain;
  while(true)
  {
    aCur++;
    if(aCur == aLenEnd)  
      return Backward(aBackRes, aCur);
    GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + aCur));
    UINT32 aNewLen = m_LongestMatchLength;
    if(aNewLen >= m_MatchLengthEdge)
      return Backward(aBackRes, aCur);
    
    UINT32 aCurPrice = m_Optimum[aCur].Price; 
    UINT32 aCurAnd1Price = aCurPrice +
        m_LiteralPrices[m_MatchFinder.GetIndexByte(aCur - m_AdditionalOffset)];
    COptimal &anOptimum = m_Optimum[aCur + 1];
    if (aCurAnd1Price < anOptimum.Price) 
    {
      anOptimum.Price = aCurAnd1Price;
      anOptimum.PosPrev = aCur;
    }
    if (aNewLen < kMatchMinLen)
      continue;
    if(aCur + aNewLen > aLenEnd)
    {
      if (aCur + aNewLen > kNumOpts - 1)
        aNewLen = kNumOpts - 1 - aCur;
      UINT32 aLenEndNew = aCur + aNewLen;
      if (aLenEnd < aLenEndNew)
      {
        for(UINT32 i = aLenEnd + 1; i <= aLenEndNew; i++)
          m_Optimum[i].Price = kIfinityPrice;
        aLenEnd = aLenEndNew;
      }
    }       
    for(UINT32 aLenTest = kMatchMinLen; aLenTest <= aNewLen; aLenTest++)
    {
      UINT16 aCurBack = m_MatchDistances[aLenTest];
      UINT32 aCurAndLenPrice = aCurPrice + 
          m_LenPrices[aLenTest - kMatchMinLen] + m_PosPrices[GetPosSlot(aCurBack)];
      COptimal &anOptimum = m_Optimum[aCur + aLenTest];
      if (aCurAndLenPrice < anOptimum.Price) 
      {
        anOptimum.Price = aCurAndLenPrice;
        anOptimum.PosPrev = aCur;
        anOptimum.BackPrev = aCurBack;
      }
    }
  }
}


void CCoder::InitStructures()
{
  memset(m_LastLevels, 0, kMaxTableSize);

  m_ValueIndex = 0;
  m_OptimumEndIndex = 0;
  m_OptimumCurrentIndex = 0;
  m_AdditionalOffset = 0;

  m_BlockStartPostion = 0;
  m_CurrentBlockUncompressedSize = 0;

  m_MainCoder.StartNewBlock();
  m_DistCoder.StartNewBlock();
 
  for(int i = 0; i < 256; i++)
    m_LiteralPrices[i] = 8;
  for(i = 0; i < kNumLenCombinations; i++)
    m_LenPrices[i] = 5 + kLenDirectBits[g_LenSlots[i]]; // test it
  for(i = 0; i < kDistTableSize; i++)
    m_PosPrices[i] = 5 + kDistDirectBits[i];
}

void CCoder::WriteBlockData(bool aWriteMode, bool anFinalBlock)
{
  m_MainCoder.AddSymbol(kReadTableNumber);
  int aMethod = WriteTables(aWriteMode, anFinalBlock);
  
  if (aWriteMode)
  {
    if(aMethod == NBlockType::kStored)
    {
      for(UINT32 i = 0; i < m_CurrentBlockUncompressedSize; i++)
      {
        BYTE aByte = m_MatchFinder.GetIndexByte(i - m_AdditionalOffset - 
              m_CurrentBlockUncompressedSize);
        m_OutStream.WriteBits(aByte, 8);
      }
    }
    else
    {
      for (UINT32 i = 0; i < m_ValueIndex; i++)
      {
        if (m_Values[i].Flag == kFlagImm)
          m_MainCoder.CodeOneValue(&m_ReverseOutStream, m_Values[i].Imm);
        else if (m_Values[i].Flag == kFlagLenPos)
        {
          UINT32 aLen = m_Values[i].Len;
          UINT32 aLenSlot = g_LenSlots[aLen];
          m_MainCoder.CodeOneValue(&m_ReverseOutStream, kMatchNumber + aLenSlot);
          m_OutStream.WriteBits(aLen - kLenStart[aLenSlot], kLenDirectBits[aLenSlot]);
          UINT32 aDist = m_Values[i].Pos;
          UINT32 aPosSlot = GetPosSlot(aDist);
          m_DistCoder.CodeOneValue(&m_ReverseOutStream, aPosSlot);
          m_OutStream.WriteBits(aDist - kDistStart[aPosSlot], kDistDirectBits[aPosSlot]);
        }
      }
      m_MainCoder.CodeOneValue(&m_ReverseOutStream, kReadTableNumber);
    }
  }
  m_MainCoder.StartNewBlock();
  m_DistCoder.StartNewBlock();
  m_ValueIndex = 0;
  UINT32 i;
  for(i = 0; i < 256; i++)
    if(m_LastLevels[i] != 0)
      m_LiteralPrices[i] = m_LastLevels[i];
    else
      m_LiteralPrices[i] = kNoLiteralDummy;

  // -------------- Normal match -----------------------------
  
  for(i = 0; i < kNumLenCombinations; i++)
  {
    UINT32 aSlot = g_LenSlots[i];
    BYTE aDummy = m_LastLevels[kMatchNumber + aSlot];
    if (aDummy != 0)
      m_LenPrices[i] = aDummy;
    else
      m_LenPrices[i] = kNoLenDummy;
    m_LenPrices[i] += kLenDirectBits[aSlot];
  }
  for(i = 0; i < kDistTableSize; i++)
  {
    BYTE aDummy = m_LastLevels[kDistTableStart + i];
    if (aDummy != 0)
      m_PosPrices[i] = aDummy;
    else
      m_PosPrices[i] = kNoPosDummy;
    m_PosPrices[i] += kDistDirectBits[i];
  }
}

void CCoder::CodeLevelTable(BYTE *aNewLevels, int aNumLevels, bool aCodeMode)
{
  int aPrevLen = 0xFF;        // last emitted length
  int aNextLen = aNewLevels[0]; // length of next code
  int aCount = 0;             // repeat aCount of the current code
  int aMaxCount = 7;          // max repeat aCount
  int aMinCount = 4;          // min repeat aCount
  if (aNextLen == 0) 
  {
    aMaxCount = 138;
    aMinCount = 3;
  }
  BYTE anOldValueInGuardElement = aNewLevels[aNumLevels]; // push guard value
  try
  {
    aNewLevels[aNumLevels] = 0xFF; // guard already set
    for (int n = 0; n < aNumLevels; n++) 
    {
      int aCurLen = aNextLen; 
      aNextLen = aNewLevels[n + 1];
      aCount++;
      if (aCount < aMaxCount && aCurLen == aNextLen) 
        continue;
      else if (aCount < aMinCount) 
        for(int i = 0; i < aCount; i++) 
        {
          int aCodeLen = aCurLen;
          if (aCodeMode)
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, aCodeLen);
          else
            m_LevelCoder.AddSymbol(aCodeLen);
        }
        else if (aCurLen != 0) 
        {
          if (aCurLen != aPrevLen) 
          {
            int aCodeLen = aCurLen;
            if (aCodeMode)
              m_LevelCoder.CodeOneValue(&m_ReverseOutStream, aCodeLen);
            else
              m_LevelCoder.AddSymbol(aCodeLen);
            aCount--;
          }
          if (aCodeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevelRepNumber);
            m_OutStream.WriteBits(aCount - 3, 2);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevelRepNumber);
        } 
        else if (aCount <= 10) 
        {
          if (aCodeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevel0Number);
            m_OutStream.WriteBits(aCount - 3, 3);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevel0Number);
        }
        else 
        {
          if (aCodeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevel0Number2);
            m_OutStream.WriteBits(aCount - 11, 7);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevel0Number2);
        }
        aCount = 0; 
        aPrevLen = aCurLen;
        if (aNextLen == 0) 
        {
          aMaxCount = 138;
          aMinCount = 3;
        } 
        else if (aCurLen == aNextLen) 
        {
          aMaxCount = 6;
          aMinCount = 3;
        } 
        else 
        {
          aMaxCount = 7;
          aMinCount = 4;
        }
    }
  }
  catch(...)
  {
    aNewLevels[aNumLevels] = anOldValueInGuardElement; // old guard 
    throw;
  }
  aNewLevels[aNumLevels] = anOldValueInGuardElement; // old guard 
}

int CCoder::WriteTables(bool aWriteMode, bool anFinalBlock)
{
  BYTE aNewLevels[kMaxTableSize + 1]; // (+ 1) for guard 

  m_MainCoder.BuildTree(&aNewLevels[0]);
  m_DistCoder.BuildTree(&aNewLevels[kDistTableStart]);

  
  memset(m_LastLevels, 0, kMaxTableSize);

  if (aWriteMode)
  {
    if(anFinalBlock)
      m_OutStream.WriteBits(NFinalBlockField::kFinalBlock, kFinalBlockFieldSize);
    else
      m_OutStream.WriteBits(NFinalBlockField::kNotFinalBlock, kFinalBlockFieldSize);
    
    m_LevelCoder.StartNewBlock();
    
    int aNumLitLenLevels = kMainTableSize;
    while(aNumLitLenLevels > kDeflateNumberOfLitLenCodesMin && aNewLevels[aNumLitLenLevels - 1] == 0)
      aNumLitLenLevels--;
    
    int aNumDistLevels = kDistTableSize;
    while(aNumDistLevels > kDeflateNumberOfDistanceCodesMin && 
      aNewLevels[kDistTableStart + aNumDistLevels - 1] == 0)
      aNumDistLevels--;
    
    
    /////////////////////////
    // First Pass

    CodeLevelTable(aNewLevels, aNumLitLenLevels, false);
    CodeLevelTable(&aNewLevels[kDistTableStart], aNumDistLevels, false);

    memcpy(m_LastLevels, aNewLevels, kMaxTableSize);
    

    BYTE aLevelLevels[kLevelTableSize];
    m_LevelCoder.BuildTree(aLevelLevels);
    
    BYTE aLevelLevelsStream[kLevelTableSize];
    int aNumLevelCodes = kDeflateNumberOfLevelCodesMin;
    int i;
    for (i = 0; i < kLevelTableSize; i++)
    {
      int aStreamPos = kCodeLengthAlphabetOrder[i];
      int aLevel = aLevelLevels[aStreamPos]; 
      if (aLevel > 0 && i >= aNumLevelCodes)
        aNumLevelCodes = i + 1;
      aLevelLevelsStream[i] = aLevel;
    }
    
    UINT32 aNumLZHuffmanBits = m_MainCoder.GetBlockBitLength();
    aNumLZHuffmanBits += m_DistCoder.GetBlockBitLength();
    aNumLZHuffmanBits += m_LevelCoder.GetBlockBitLength();
    aNumLZHuffmanBits += kDeflateNumberOfLengthCodesFieldSize +
      kDeflateNumberOfDistanceCodesFieldSize +
      kDeflateNumberOfLevelCodesFieldSize;
    aNumLZHuffmanBits += aNumLevelCodes * kDeflateLevelCodeFieldSize;

    UINT32 aNextBitPosition = 
        (m_OutStream.GetBitPosition() + kBlockTypeFieldSize) % 8;
    UINT32 aNumBitsForAlign = aNextBitPosition > 0 ? (8 - aNextBitPosition): 0;

    UINT32 aNumStoreBits = aNumBitsForAlign + (2 * sizeof(UINT16)) * 8;
    aNumStoreBits += m_CurrentBlockUncompressedSize * 8;
    if(aNumStoreBits < aNumLZHuffmanBits)
    {
      m_OutStream.WriteBits(NBlockType::kStored, kBlockTypeFieldSize); // test it
      m_OutStream.WriteBits(0, aNumBitsForAlign); // test it
      UINT16 aCurrentBlockUncompressedSize = UINT16(m_CurrentBlockUncompressedSize);
      UINT16 aCurrentBlockUncompressedSizeNot = ~aCurrentBlockUncompressedSize;
      m_OutStream.WriteBits(aCurrentBlockUncompressedSize, kDeflateStoredBlockLengthFieldSizeSize);
      m_OutStream.WriteBits(aCurrentBlockUncompressedSizeNot, kDeflateStoredBlockLengthFieldSizeSize);
      return NBlockType::kStored;
    }
    else
    {
      m_OutStream.WriteBits(NBlockType::kDynamicHuffman, kBlockTypeFieldSize);
      m_OutStream.WriteBits(aNumLitLenLevels - kDeflateNumberOfLitLenCodesMin, kDeflateNumberOfLengthCodesFieldSize);
      m_OutStream.WriteBits(aNumDistLevels - kDeflateNumberOfDistanceCodesMin, 
        kDeflateNumberOfDistanceCodesFieldSize);
      m_OutStream.WriteBits(aNumLevelCodes - kDeflateNumberOfLevelCodesMin, 
        kDeflateNumberOfLevelCodesFieldSize);
      
      for (i = 0; i < aNumLevelCodes; i++)
        m_OutStream.WriteBits(aLevelLevelsStream[i], kDeflateLevelCodeFieldSize);
      
      /////////////////////////
      // Second Pass

      CodeLevelTable(aNewLevels, aNumLitLenLevels, true);
      CodeLevelTable(&aNewLevels[kDistTableStart], aNumDistLevels, true);
      return NBlockType::kDynamicHuffman;
    }
  }
  else
    memcpy(m_LastLevels, aNewLevels, kMaxTableSize);
  return -1;
}

HRESULT CCoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (!m_Created)
  {
    RETURN_IF_NOT_S_OK(Create());
    m_Created = true;
  }

  UINT64 aNowPos = 0;
  m_FinderPos = 0;

  RETURN_IF_NOT_S_OK(m_MatchFinder.Init(anInStream));
  m_OutStream.Init(anOutStream);
  m_ReverseOutStream.Init(&m_OutStream);

  CCoderReleaser aCoderReleaser(this);
  InitStructures();

  while(true)
  {
    int aCurrentPassIndex = 0;
    bool aNoMoreBytes;
    while (true)
    {
      while(true)
      {
        aNoMoreBytes = (m_AdditionalOffset == 0 && m_MatchFinder.GetNumAvailableBytes() == 0);
  
        if (((m_CurrentBlockUncompressedSize >= kBlockUncompressedSizeThreshold || 
                 m_ValueIndex >= kValueBlockSize) && 
              (m_OptimumEndIndex == m_OptimumCurrentIndex)) 
            || aNoMoreBytes)
          break;
        UINT32 aPos;
        UINT32 aLen = GetOptimal(aPos);
        if (aLen >= kMatchMinLen)
        {
          UINT32 aNewLen = aLen - kMatchMinLen;
          m_Values[m_ValueIndex].Flag = kFlagLenPos;
          m_Values[m_ValueIndex].Len = BYTE(aNewLen);
          UINT32 aLenSlot = g_LenSlots[aNewLen];
          m_MainCoder.AddSymbol(kMatchNumber + aLenSlot);
          m_Values[m_ValueIndex].Pos = UINT16(aPos);
          UINT32 aPosSlot = GetPosSlot(aPos);
          m_DistCoder.AddSymbol(aPosSlot);
        }
        else if (aLen == 1)  
        {
          BYTE aByte = m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset);
          aLen = 1;
          m_MainCoder.AddSymbol(aByte);
          m_Values[m_ValueIndex].Flag = kFlagImm;
          m_Values[m_ValueIndex].Imm = aByte;
        }
        else
          throw 12112342;
        m_ValueIndex++;
        m_AdditionalOffset -= aLen;
        aNowPos += aLen;
        m_CurrentBlockUncompressedSize += aLen;
        
      }
      aCurrentPassIndex++;
      bool aWriteMode = (aCurrentPassIndex == m_NumPasses);
      WriteBlockData(aWriteMode, aNoMoreBytes);
      if (aWriteMode)
        break;
      aNowPos = m_BlockStartPostion;
      m_AdditionalOffset = UINT32(m_FinderPos - m_BlockStartPostion);
      m_CurrentBlockUncompressedSize = 0;
    }
    m_BlockStartPostion += m_CurrentBlockUncompressedSize;
    m_CurrentBlockUncompressedSize = 0;
    if (aProgress != NULL)
    {
      UINT64 aPackSize = m_OutStream.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aNowPos, &aPackSize));
    }
    if (aNoMoreBytes)
      break;
  }
  return  m_OutStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(CMatchFinderException &anOutWriteException)
  {
    return anOutWriteException.m_Result;
  }
  catch(const NStream::COutByteWriteException &anException)
  {
    return anException.m_Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}

/*
STDMETHODIMP CCoder::InitMatchFinder(IInWindowStreamMatch *aMatchFinder)
{
  m_MatchFinder = aMatchFinder;
  return S_OK;
}
*/

}}
