// DeflateEncoder.cpp

#include "StdAfx.h"

#include "DeflateEncoder.h"
#include "DeflateConst.h"

#include "Windows/Defs.h"
#include "Common/ComTry.h"
#include "../../../Common/Alloc.h"
#include "../LZ/BinTree/BinTree3Z.h"

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

class CMatchFinderException
{
public:
  HRESULT m_Result;
  CMatchFinderException(HRESULT result): m_Result (result) {}
};

static const int kValueBlockSize = 0x2000;

static const int kMaxCodeBitLength = 15;
static const int kMaxLevelBitLength = 7;

static const Byte kFlagImm     = 0;
static const Byte kFlagLenPos  = 4;

static const UInt32 kMaxUncompressedBlockSize = 0xFFFF; // test it !!!

static const UInt32 kBlockUncompressedSizeThreshold = 
    kMaxUncompressedBlockSize - kMatchMaxLen32 - kNumOpts;

static const int kNumGoodBacks = 0x10000; 

static Byte kNoLiteralDummy = 13;
static Byte kNoLenDummy = 13;
static Byte kNoPosDummy = 6;

static Byte g_LenSlots[kNumLenCombinations32];
static Byte g_FastPos[1 << 9];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    int i;
    for(i = 0; i < kLenTableSize; i++)
    {
      int c = kLenStart32[i];
      int j = 1 << kLenDirectBits32[i];
      for(int k = 0; k < j; k++, c++)
        g_LenSlots[c] = (Byte)i;
    }
    
    const int kFastSlots = 18;
    int c = 0;
    for (Byte slotFast = 0; slotFast < kFastSlots; slotFast++)
    {
      UInt32 k = (1 << kDistDirectBits[slotFast]);
      for (UInt32 j = 0; j < k; j++, c++)
        g_FastPos[c] = slotFast;
    }
  }
};

static CFastPosInit g_FastPosInit;


inline UInt32 GetPosSlot(UInt32 pos)
{
  //  for (UInt32 i = 1; pos >= kDistStart[i]; i++);
  //    return i - 1;
  if (pos < 0x200)
    return g_FastPos[pos];
  return g_FastPos[pos >> 8] + 16;
}

CCoder::CCoder(bool deflate64Mode):
  _deflate64Mode(deflate64Mode),
  m_MainCoder(kMainTableSize, 
      deflate64Mode ? kLenDirectBits64 : kLenDirectBits32, 
      kMatchNumber, kMaxCodeBitLength),
  m_DistCoder(deflate64Mode ? kDistTableSize64 : kDistTableSize32, kDistDirectBits, 0, kMaxCodeBitLength),
  m_LevelCoder(kLevelTableSize, kLevelDirectBits, 0, kMaxLevelBitLength),
  m_NumPasses(1),
  m_NumFastBytes(32),
  m_OnePosMatchesMemory(0),
  m_OnePosMatchesArray(0),
  m_MatchDistances(0),
  m_Created(false),
  m_Values(0)
{
  m_MatchMaxLen = deflate64Mode ? kMatchMaxLen64 : kMatchMaxLen32;
  m_NumLenCombinations = deflate64Mode ? kNumLenCombinations64 : 
    kNumLenCombinations32;
  m_LenStart = deflate64Mode ? kLenStart64 : kLenStart32;
  m_LenDirectBits = deflate64Mode ? kLenDirectBits64 : kLenDirectBits32;
}

HRESULT CCoder::Create()
{
  COM_TRY_BEGIN
  if (!m_MatchFinder)
  {
    m_MatchFinder = new NBT3Z::CMatchFinderBinTree;
    if (m_MatchFinder == 0)
      return E_OUTOFMEMORY;
  }
  if (m_Values == 0)
  {
    m_Values = (CCodeValue *)MyAlloc((kValueBlockSize + kNumOpts) * sizeof(CCodeValue));
    if (m_Values == 0)
      return E_OUTOFMEMORY;
  }
  RINOK(m_MatchFinder->Create(_deflate64Mode ? kHistorySize64 : kHistorySize32, 
      kNumOpts + kNumGoodBacks, m_NumFastBytes, m_MatchMaxLen - m_NumFastBytes));
  if (!m_OutStream.Create(1 << 20))
    return E_OUTOFMEMORY;
  m_MatchLengthEdge = m_NumFastBytes + 1;

  Free();
  if (m_NumPasses > 1)
  {
    m_OnePosMatchesMemory = (UInt16 *)BigAlloc(kNumGoodBacks * (m_NumFastBytes + 1) * sizeof(UInt16));
    if (m_OnePosMatchesMemory == 0)
      return E_OUTOFMEMORY;
    m_OnePosMatchesArray = (COnePosMatches *)MyAlloc(kNumGoodBacks * sizeof(COnePosMatches));
    if (m_OnePosMatchesArray == 0)
      return E_OUTOFMEMORY;
    UInt16 *goodBacksWordsCurrent = m_OnePosMatchesMemory;
    for(int i = 0; i < kNumGoodBacks; i++, goodBacksWordsCurrent += (m_NumFastBytes + 1))
      m_OnePosMatchesArray[i].Init(goodBacksWordsCurrent);
  }
  else
  {
    m_MatchDistances = (UInt16 *)MyAlloc((m_NumFastBytes + 1) * sizeof(UInt16));
    if (m_MatchDistances == 0)
      return E_OUTOFMEMORY;
  }
  return S_OK;
  COM_TRY_END
}

// ICompressSetEncoderProperties2
HRESULT CCoder::BaseSetEncoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
{
  for(UInt32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &property = properties[i]; 
    switch(propIDs[i])
    {
      case NCoderPropID::kNumPasses:
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumPasses = property.ulVal;
        if(m_NumPasses == 0 || m_NumPasses > 255)
          return E_INVALIDARG;
        break;
      case NCoderPropID::kNumFastBytes:
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumFastBytes = property.ulVal;
        if(m_NumFastBytes < 3 || m_NumFastBytes > m_MatchMaxLen)
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
      BigFree(m_OnePosMatchesMemory);
      MyFree(m_OnePosMatchesArray);
    }
    else
      MyFree(m_MatchDistances);
  }
}

CCoder::~CCoder()
{
  Free();
  MyFree(m_Values);
}

void CCoder::ReadGoodBacks()
{
  UInt32 goodIndex;
  if (m_NumPasses > 1)
  {
    goodIndex = m_FinderPos % kNumGoodBacks;
    m_MatchDistances = m_OnePosMatchesArray[goodIndex].MatchDistances;
  }
  UInt32 distanceTmp[kMatchMaxLen32 + 1];
  UInt32 len = m_MatchFinder->GetLongestMatch(distanceTmp);
  for(UInt32 i = kMatchMinLen; i <= len; i++)
    m_MatchDistances[i] = (UInt16)distanceTmp[i];

  m_LongestMatchDistance = m_MatchDistances[len];
  if (len == m_NumFastBytes && m_NumFastBytes != m_MatchMaxLen)
    m_LongestMatchLength = len + m_MatchFinder->GetMatchLen(len, 
        m_LongestMatchDistance, m_MatchMaxLen - len);
  else
    m_LongestMatchLength = len;
  if (m_NumPasses > 1)
  {
    m_OnePosMatchesArray[goodIndex].LongestMatchDistance = UInt16(m_LongestMatchDistance);
    m_OnePosMatchesArray[goodIndex].LongestMatchLength = UInt16(m_LongestMatchLength);
  }
  HRESULT result = m_MatchFinder->MovePos();
  if (result != S_OK)
    throw CMatchFinderException(result);
  m_FinderPos++;
  m_AdditionalOffset++;
}

void CCoder::GetBacks(UInt32 pos)
{
  if(pos == m_FinderPos)
    ReadGoodBacks();
  else
  {
    if (m_NumPasses == 1)
    {
      if(pos + 1 == m_FinderPos) 
        return;
      throw 1932;   
    }
    else
    {
      UInt32 goodIndex = pos % kNumGoodBacks;
      m_MatchDistances = m_OnePosMatchesArray[goodIndex].MatchDistances;
      m_LongestMatchDistance = m_OnePosMatchesArray[goodIndex].LongestMatchDistance;
      m_LongestMatchLength = m_OnePosMatchesArray[goodIndex].LongestMatchLength;
    }
  }
}


void CCoder::MovePos(UInt32 num)
{
  if (m_NumPasses > 1)
  {
    for(UInt32 i = 0; i < num; i++)
      GetBacks(UInt32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + i + 1));
  }
  else
  {
    for (;num > 0; num--)
    {
      m_MatchFinder->DummyLongestMatch();
      HRESULT result = m_MatchFinder->MovePos();
      if (result != S_OK)
        throw CMatchFinderException(result);
      m_FinderPos++;
      m_AdditionalOffset++;
    }
  }
}

static const UInt32 kIfinityPrice = 0xFFFFFFF;

UInt32 CCoder::Backward(UInt32 &backRes, UInt32 cur)
{
  m_OptimumEndIndex = cur;
  UInt32 posMem = m_Optimum[cur].PosPrev;
  UInt16 backMem = m_Optimum[cur].BackPrev;
  do
  {
    UInt32 posPrev = posMem;
    UInt16 backCur = backMem;
    backMem = m_Optimum[posPrev].BackPrev;
    posMem = m_Optimum[posPrev].PosPrev;
    m_Optimum[posPrev].BackPrev = backCur;
    m_Optimum[posPrev].PosPrev = (UInt16)cur;
    cur = posPrev;
  }
  while(cur > 0);
  backRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex  = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex; 
}

UInt32 CCoder::GetOptimal(UInt32 &backRes)
{
  if(m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    UInt32 len = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    backRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return len;
  }
  m_OptimumCurrentIndex = 0;
  m_OptimumEndIndex = 0;
  
  GetBacks(UInt32(m_BlockStartPostion + m_CurrentBlockUncompressedSize));

  UInt32 lenMain = m_LongestMatchLength;
  UInt32 backMain = m_LongestMatchDistance;

  if(lenMain < kMatchMinLen)
    return 1;
  if(lenMain >= m_MatchLengthEdge)
  {
    backRes = backMain; 
    MovePos(lenMain - 1);
    return lenMain;
  }
  m_Optimum[1].Price = m_LiteralPrices[m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset)];
  m_Optimum[1].PosPrev = 0;

  m_Optimum[2].Price = kIfinityPrice;
  m_Optimum[2].PosPrev = 1;

  for(UInt32 i = kMatchMinLen; i <= lenMain; i++)
  {
    m_Optimum[i].PosPrev = 0;
    m_Optimum[i].BackPrev = m_MatchDistances[i];
    m_Optimum[i].Price = m_LenPrices[i - kMatchMinLen] + m_PosPrices[GetPosSlot(m_MatchDistances[i])];
  }


  UInt32 cur = 0;
  UInt32 lenEnd = lenMain;
  while(true)
  {
    cur++;
    if(cur == lenEnd)  
      return Backward(backRes, cur);
    GetBacks(UInt32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + cur));
    UInt32 newLen = m_LongestMatchLength;
    if(newLen >= m_MatchLengthEdge)
      return Backward(backRes, cur);
    
    UInt32 curPrice = m_Optimum[cur].Price; 
    UInt32 curAnd1Price = curPrice +
        m_LiteralPrices[m_MatchFinder->GetIndexByte(cur - m_AdditionalOffset)];
    COptimal &optimum = m_Optimum[cur + 1];
    if (curAnd1Price < optimum.Price) 
    {
      optimum.Price = curAnd1Price;
      optimum.PosPrev = (UInt16)cur;
    }
    if (newLen < kMatchMinLen)
      continue;
    if(cur + newLen > lenEnd)
    {
      if (cur + newLen > kNumOpts - 1)
        newLen = kNumOpts - 1 - cur;
      UInt32 lenEndNew = cur + newLen;
      if (lenEnd < lenEndNew)
      {
        for(UInt32 i = lenEnd + 1; i <= lenEndNew; i++)
          m_Optimum[i].Price = kIfinityPrice;
        lenEnd = lenEndNew;
      }
    }       
    for(UInt32 lenTest = kMatchMinLen; lenTest <= newLen; lenTest++)
    {
      UInt16 curBack = m_MatchDistances[lenTest];
      UInt32 curAndLenPrice = curPrice + 
          m_LenPrices[lenTest - kMatchMinLen] + m_PosPrices[GetPosSlot(curBack)];
      COptimal &optimum = m_Optimum[cur + lenTest];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = (UInt16)cur;
        optimum.BackPrev = curBack;
      }
    }
  }
}


void CCoder::InitStructures()
{
  memset(m_LastLevels, 0, kMaxTableSize64);

  m_ValueIndex = 0;
  m_OptimumEndIndex = 0;
  m_OptimumCurrentIndex = 0;
  m_AdditionalOffset = 0;

  m_BlockStartPostion = 0;
  m_CurrentBlockUncompressedSize = 0;

  m_MainCoder.StartNewBlock();
  m_DistCoder.StartNewBlock();
 
  UInt32 i;
  for(i = 0; i < 256; i++)
    m_LiteralPrices[i] = 8;
  for(i = 0; i < m_NumLenCombinations; i++)
    m_LenPrices[i] = (Byte)(5 + m_LenDirectBits[g_LenSlots[i]]); // test it
  for(i = 0; i < kDistTableSize64; i++)
    m_PosPrices[i] = (Byte)(5 + kDistDirectBits[i]);
}

void CCoder::WriteBlockData(bool writeMode, bool finalBlock)
{
  m_MainCoder.AddSymbol(kReadTableNumber);
  int method = WriteTables(writeMode, finalBlock);
  
  if (writeMode)
  {
    if(method == NBlockType::kStored)
    {
      for(UInt32 i = 0; i < m_CurrentBlockUncompressedSize; i++)
      {
        Byte b = m_MatchFinder->GetIndexByte(i - m_AdditionalOffset - 
              m_CurrentBlockUncompressedSize);
        m_OutStream.WriteBits(b, 8);
      }
    }
    else
    {
      for (UInt32 i = 0; i < m_ValueIndex; i++)
      {
        if (m_Values[i].Flag == kFlagImm)
          m_MainCoder.CodeOneValue(&m_ReverseOutStream, m_Values[i].Imm);
        else if (m_Values[i].Flag == kFlagLenPos)
        {
          UInt32 len = m_Values[i].Len;
          UInt32 lenSlot = g_LenSlots[len];
          m_MainCoder.CodeOneValue(&m_ReverseOutStream, kMatchNumber + lenSlot);
          m_OutStream.WriteBits(len - m_LenStart[lenSlot], m_LenDirectBits[lenSlot]);
          UInt32 dist = m_Values[i].Pos;
          UInt32 posSlot = GetPosSlot(dist);
          m_DistCoder.CodeOneValue(&m_ReverseOutStream, posSlot);
          m_OutStream.WriteBits(dist - kDistStart[posSlot], kDistDirectBits[posSlot]);
        }
      }
      m_MainCoder.CodeOneValue(&m_ReverseOutStream, kReadTableNumber);
    }
  }
  m_MainCoder.StartNewBlock();
  m_DistCoder.StartNewBlock();
  m_ValueIndex = 0;
  UInt32 i;
  for(i = 0; i < 256; i++)
    if(m_LastLevels[i] != 0)
      m_LiteralPrices[i] = m_LastLevels[i];
    else
      m_LiteralPrices[i] = kNoLiteralDummy;

  // -------------- Normal match -----------------------------
  
  for(i = 0; i < m_NumLenCombinations; i++)
  {
    UInt32 slot = g_LenSlots[i];
    Byte dummy = m_LastLevels[kMatchNumber + slot];
    if (dummy != 0)
      m_LenPrices[i] = dummy;
    else
      m_LenPrices[i] = kNoLenDummy;
    m_LenPrices[i] += m_LenDirectBits[slot];
  }
  for(i = 0; i < kDistTableSize64; i++)
  {
    Byte dummy = m_LastLevels[kDistTableStart + i];
    if (dummy != 0)
      m_PosPrices[i] = dummy;
    else
      m_PosPrices[i] = kNoPosDummy;
    m_PosPrices[i] += kDistDirectBits[i];
  }
}

void CCoder::CodeLevelTable(Byte *newLevels, int numLevels, bool codeMode)
{
  int prevLen = 0xFF;        // last emitted length
  int nextLen = newLevels[0]; // length of next code
  int count = 0;             // repeat count of the current code
  int maxCount = 7;          // max repeat count
  int minCount = 4;          // min repeat count
  if (nextLen == 0) 
  {
    maxCount = 138;
    minCount = 3;
  }
  Byte oldValueInGuardElement = newLevels[numLevels]; // push guard value
  try
  {
    newLevels[numLevels] = 0xFF; // guard already set
    for (int n = 0; n < numLevels; n++) 
    {
      int curLen = nextLen; 
      nextLen = newLevels[n + 1];
      count++;
      if (count < maxCount && curLen == nextLen) 
        continue;
      else if (count < minCount) 
        for(int i = 0; i < count; i++) 
        {
          int codeLen = curLen;
          if (codeMode)
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, codeLen);
          else
            m_LevelCoder.AddSymbol(codeLen);
        }
        else if (curLen != 0) 
        {
          if (curLen != prevLen) 
          {
            int codeLen = curLen;
            if (codeMode)
              m_LevelCoder.CodeOneValue(&m_ReverseOutStream, codeLen);
            else
              m_LevelCoder.AddSymbol(codeLen);
            count--;
          }
          if (codeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevelRepNumber);
            m_OutStream.WriteBits(count - 3, 2);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevelRepNumber);
        } 
        else if (count <= 10) 
        {
          if (codeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevel0Number);
            m_OutStream.WriteBits(count - 3, 3);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevel0Number);
        }
        else 
        {
          if (codeMode)
          {
            m_LevelCoder.CodeOneValue(&m_ReverseOutStream, kTableLevel0Number2);
            m_OutStream.WriteBits(count - 11, 7);
          }
          else
            m_LevelCoder.AddSymbol(kTableLevel0Number2);
        }
        count = 0; 
        prevLen = curLen;
        if (nextLen == 0) 
        {
          maxCount = 138;
          minCount = 3;
        } 
        else if (curLen == nextLen) 
        {
          maxCount = 6;
          minCount = 3;
        } 
        else 
        {
          maxCount = 7;
          minCount = 4;
        }
    }
  }
  catch(...)
  {
    newLevels[numLevels] = oldValueInGuardElement; // old guard 
    throw;
  }
  newLevels[numLevels] = oldValueInGuardElement; // old guard 
}

int CCoder::WriteTables(bool writeMode, bool finalBlock)
{
  Byte newLevels[kMaxTableSize64 + 1]; // (+ 1) for guard 

  m_MainCoder.BuildTree(&newLevels[0]);
  m_DistCoder.BuildTree(&newLevels[kDistTableStart]);

  
  memset(m_LastLevels, 0, kMaxTableSize64);

  if (writeMode)
  {
    if(finalBlock)
      m_OutStream.WriteBits(NFinalBlockField::kFinalBlock, kFinalBlockFieldSize);
    else
      m_OutStream.WriteBits(NFinalBlockField::kNotFinalBlock, kFinalBlockFieldSize);
    
    m_LevelCoder.StartNewBlock();
    
    int numLitLenLevels = kMainTableSize;
    while(numLitLenLevels > kDeflateNumberOfLitLenCodesMin && newLevels[numLitLenLevels - 1] == 0)
      numLitLenLevels--;
    
    int numDistLevels = _deflate64Mode ? kDistTableSize64 : kDistTableSize32;
    while(numDistLevels > kDeflateNumberOfDistanceCodesMin && 
      newLevels[kDistTableStart + numDistLevels - 1] == 0)
      numDistLevels--;
    
    
    /////////////////////////
    // First Pass

    CodeLevelTable(newLevels, numLitLenLevels, false);
    CodeLevelTable(&newLevels[kDistTableStart], numDistLevels, false);

    memcpy(m_LastLevels, newLevels, kMaxTableSize64);
    

    Byte levelLevels[kLevelTableSize];
    m_LevelCoder.BuildTree(levelLevels);
    
    Byte levelLevelsStream[kLevelTableSize];
    int numLevelCodes = kDeflateNumberOfLevelCodesMin;
    int i;
    for (i = 0; i < kLevelTableSize; i++)
    {
      int streamPos = kCodeLengthAlphabetOrder[i];
      Byte level = levelLevels[streamPos]; 
      if (level > 0 && i >= numLevelCodes)
        numLevelCodes = i + 1;
      levelLevelsStream[i] = level;
    }
    
    UInt32 numLZHuffmanBits = m_MainCoder.GetBlockBitLength();
    numLZHuffmanBits += m_DistCoder.GetBlockBitLength();
    numLZHuffmanBits += m_LevelCoder.GetBlockBitLength();
    numLZHuffmanBits += kDeflateNumberOfLengthCodesFieldSize +
      kDeflateNumberOfDistanceCodesFieldSize +
      kDeflateNumberOfLevelCodesFieldSize;
    numLZHuffmanBits += numLevelCodes * kDeflateLevelCodeFieldSize;

    UInt32 nextBitPosition = 
        (m_OutStream.GetBitPosition() + kBlockTypeFieldSize) % 8;
    UInt32 numBitsForAlign = nextBitPosition > 0 ? (8 - nextBitPosition): 0;

    UInt32 numStoreBits = numBitsForAlign + (2 * 2) * 8;
    numStoreBits += m_CurrentBlockUncompressedSize * 8;
    if(numStoreBits < numLZHuffmanBits)
    {
      m_OutStream.WriteBits(NBlockType::kStored, kBlockTypeFieldSize); // test it
      m_OutStream.WriteBits(0, numBitsForAlign); // test it
      UInt16 currentBlockUncompressedSize = UInt16(m_CurrentBlockUncompressedSize);
      UInt16 currentBlockUncompressedSizeNot = ~currentBlockUncompressedSize;
      m_OutStream.WriteBits(currentBlockUncompressedSize, kDeflateStoredBlockLengthFieldSizeSize);
      m_OutStream.WriteBits(currentBlockUncompressedSizeNot, kDeflateStoredBlockLengthFieldSizeSize);
      return NBlockType::kStored;
    }
    else
    {
      m_OutStream.WriteBits(NBlockType::kDynamicHuffman, kBlockTypeFieldSize);
      m_OutStream.WriteBits(numLitLenLevels - kDeflateNumberOfLitLenCodesMin, kDeflateNumberOfLengthCodesFieldSize);
      m_OutStream.WriteBits(numDistLevels - kDeflateNumberOfDistanceCodesMin, 
        kDeflateNumberOfDistanceCodesFieldSize);
      m_OutStream.WriteBits(numLevelCodes - kDeflateNumberOfLevelCodesMin, 
        kDeflateNumberOfLevelCodesFieldSize);
      
      for (i = 0; i < numLevelCodes; i++)
        m_OutStream.WriteBits(levelLevelsStream[i], kDeflateLevelCodeFieldSize);
      
      /////////////////////////
      // Second Pass

      CodeLevelTable(newLevels, numLitLenLevels, true);
      CodeLevelTable(&newLevels[kDistTableStart], numDistLevels, true);
      return NBlockType::kDynamicHuffman;
    }
  }
  else
    memcpy(m_LastLevels, newLevels, kMaxTableSize64);
  return -1;
}

HRESULT CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!m_Created)
  {
    RINOK(Create());
    m_Created = true;
  }

  UInt64 nowPos = 0;
  m_FinderPos = 0;

  RINOK(m_MatchFinder->Init(inStream));
  m_OutStream.SetStream(outStream);
  m_OutStream.Init();
  m_ReverseOutStream.Init(&m_OutStream);

  CCoderReleaser coderReleaser(this);
  InitStructures();

  while(true)
  {
    int currentPassIndex = 0;
    bool noMoreBytes;
    while (true)
    {
      while(true)
      {
        noMoreBytes = (m_AdditionalOffset == 0 && m_MatchFinder->GetNumAvailableBytes() == 0);
  
        if (((m_CurrentBlockUncompressedSize >= kBlockUncompressedSizeThreshold || 
                 m_ValueIndex >= kValueBlockSize) && 
              (m_OptimumEndIndex == m_OptimumCurrentIndex)) 
            || noMoreBytes)
          break;
        UInt32 pos;
        UInt32 len = GetOptimal(pos);
        if (len >= kMatchMinLen)
        {
          UInt32 newLen = len - kMatchMinLen;
          m_Values[m_ValueIndex].Flag = kFlagLenPos;
          m_Values[m_ValueIndex].Len = Byte(newLen);
          UInt32 lenSlot = g_LenSlots[newLen];
          m_MainCoder.AddSymbol(kMatchNumber + lenSlot);
          m_Values[m_ValueIndex].Pos = UInt16(pos);
          UInt32 posSlot = GetPosSlot(pos);
          m_DistCoder.AddSymbol(posSlot);
        }
        else if (len == 1)  
        {
          Byte b = m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset);
          len = 1;
          m_MainCoder.AddSymbol(b);
          m_Values[m_ValueIndex].Flag = kFlagImm;
          m_Values[m_ValueIndex].Imm = b;
        }
        else
          throw 12112342;
        m_ValueIndex++;
        m_AdditionalOffset -= len;
        nowPos += len;
        m_CurrentBlockUncompressedSize += len;
        
      }
      currentPassIndex++;
      bool writeMode = (currentPassIndex == m_NumPasses);
      WriteBlockData(writeMode, noMoreBytes);
      if (writeMode)
        break;
      nowPos = m_BlockStartPostion;
      m_AdditionalOffset = UInt32(m_FinderPos - m_BlockStartPostion);
      m_CurrentBlockUncompressedSize = 0;
    }
    m_BlockStartPostion += m_CurrentBlockUncompressedSize;
    m_CurrentBlockUncompressedSize = 0;
    if (progress != NULL)
    {
      UInt64 packSize = m_OutStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&nowPos, &packSize));
    }
    if (noMoreBytes)
      break;
  }
  return  m_OutStream.Flush();
}

HRESULT CCoder::BaseCode(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(CMatchFinderException &e) {  return e.m_Result;   }
  catch(const COutBufferException &e) { return e.ErrorCode; }
  catch(...) { return E_FAIL; }
}

STDMETHODIMP CCOMCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

STDMETHODIMP CCOMCoder::SetCoderProperties(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
  { return BaseSetEncoderProperties2(propIDs, properties, numProperties); }

STDMETHODIMP CCOMCoder64::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

STDMETHODIMP CCOMCoder64::SetCoderProperties(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
  { return BaseSetEncoderProperties2(propIDs, properties, numProperties); }

}}}
