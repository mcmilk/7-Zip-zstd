// Deflate/Encoder.cpp

#include "StdAfx.h"

#include "Encoder.h"
#include "Const.h"

#include "Windows/Defs.h"
#include "Windows/COMTRY.h"
#include "../MatchFinder/BinTree/BinTree3ZMain.h"

namespace NDeflate {
namespace NEncoder {

class CMatchFinderException
{
public:
  HRESULT m_Result;
  CMatchFinderException(HRESULT result): m_Result (result) {}
};

static const kValueBlockSize = 0x2000;

static const kMaxCodeBitLength = 15;
static const kMaxLevelBitLength = 7;

static const BYTE kFlagImm     = 0;
static const BYTE kFlagLenPos  = 4;

static const UINT32 kMaxUncompressedBlockSize = 0xFFFF; // test it !!!

static const UINT32 kBlockUncompressedSizeThreshold = 
    kMaxUncompressedBlockSize - kMatchMaxLen32 - kNumOpts;

static const kNumGoodBacks = 0x10000; 

static BYTE kNoLiteralDummy = 13;
static BYTE kNoLenDummy = 13;
static BYTE kNoPosDummy = 6;

static BYTE g_LenSlots[kNumLenCombinations32];
static BYTE g_FastPos[1 << 9];

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
        g_LenSlots[c] = i;
    }
    
    const kFastSlots = 18;
    int c = 0;
    for (BYTE slotFast = 0; slotFast < kFastSlots; slotFast++)
    {
      UINT32 k = (1 << kDistDirectBits[slotFast]);
      for (UINT32 j = 0; j < k; j++, c++)
        g_FastPos[c] = slotFast;
    }
  }
};

static CFastPosInit g_FastPosInit;


inline UINT32 GetPosSlot(UINT32 pos)
{
  //  for (UINT32 i = 1; pos >= kDistStart[i]; i++);
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

  m_Values = new CCodeValue[kValueBlockSize + kNumOpts];
}

HRESULT CCoder::Create()
{
  COM_TRY_BEGIN
  m_MatchFinder.Create(
    _deflate64Mode ? kHistorySize64 : kHistorySize32, 
    kNumOpts + kNumGoodBacks, m_NumFastBytes, 
    m_MatchMaxLen - m_NumFastBytes);
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
    UINT16 *goodBacksWordsCurrent = m_OnePosMatchesMemory;
    for(int i = 0; i < kNumGoodBacks; i++, goodBacksWordsCurrent += (m_NumFastBytes + 1))
      m_OnePosMatchesArray[i].Init(goodBacksWordsCurrent);
  }
  else
    m_MatchDistances = new UINT16[m_NumFastBytes + 1];
  return S_OK;
  COM_TRY_END
}

// ICompressSetEncoderProperties2
HRESULT CCoder::BaseSetEncoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
{
  for(UINT32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &property = properties[i]; 
    switch(propIDs[i])
    {
      case NEncodingProperies::kNumPasses:
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumPasses = property.ulVal;
        if(m_NumPasses == 0 || m_NumPasses > 255)
          return E_INVALIDARG;
        break;
      case NEncodingProperies::kNumFastBytes:
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
  UINT32 goodIndex;
  if (m_NumPasses > 1)
  {
    goodIndex = m_FinderPos % kNumGoodBacks;
    m_MatchDistances = m_OnePosMatchesArray[goodIndex].MatchDistances;
  }
  UINT32 distanceTmp[kMatchMaxLen32 + 1];
  UINT32 len = m_MatchFinder.GetLongestMatch(distanceTmp);
  for(UINT32 i = kMatchMinLen; i <= len; i++)
    m_MatchDistances[i] = distanceTmp[i];

  m_LongestMatchDistance = m_MatchDistances[len];
  if (len == m_NumFastBytes && m_NumFastBytes != m_MatchMaxLen)
    m_LongestMatchLength = len + m_MatchFinder.GetMatchLen(len, 
        m_LongestMatchDistance, m_MatchMaxLen - len);
  else
    m_LongestMatchLength = len;
  if (m_NumPasses > 1)
  {
    m_OnePosMatchesArray[goodIndex].LongestMatchDistance = UINT16(m_LongestMatchDistance);
    m_OnePosMatchesArray[goodIndex].LongestMatchLength = UINT16(m_LongestMatchLength);
  }
  HRESULT result = m_MatchFinder.MovePos();
  if (result != S_OK)
    throw CMatchFinderException(result);
  m_FinderPos++;
  m_AdditionalOffset++;
}

void CCoder::GetBacks(UINT32 pos)
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
      UINT32 goodIndex = pos % kNumGoodBacks;
      m_MatchDistances = m_OnePosMatchesArray[goodIndex].MatchDistances;
      m_LongestMatchDistance = m_OnePosMatchesArray[goodIndex].LongestMatchDistance;
      m_LongestMatchLength = m_OnePosMatchesArray[goodIndex].LongestMatchLength;
    }
  }
}


void CCoder::MovePos(UINT32 num)
{
  if (m_NumPasses > 1)
  {
    for(UINT32 i = 0; i < num; i++)
      GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + i + 1));
  }
  else
  {
    for (;num > 0; num--)
    {
      m_MatchFinder.DummyLongestMatch();
      HRESULT result = m_MatchFinder.MovePos();
      if (result != S_OK)
        throw CMatchFinderException(result);
      m_FinderPos++;
      m_AdditionalOffset++;
    }
  }
}

static const kIfinityPrice = 0xFFFFFFF;

UINT32 CCoder::Backward(UINT32 &backRes, UINT32 cur)
{
  m_OptimumEndIndex = cur;
  UINT32 posMem = m_Optimum[cur].PosPrev;
  UINT16 backMem = m_Optimum[cur].BackPrev;
  do
  {
    UINT32 posPrev = posMem;
    UINT16 backCur = backMem;
    backMem = m_Optimum[posPrev].BackPrev;
    posMem = m_Optimum[posPrev].PosPrev;
    m_Optimum[posPrev].BackPrev = backCur;
    m_Optimum[posPrev].PosPrev = cur;
    cur = posPrev;
  }
  while(cur > 0);
  backRes = m_Optimum[0].BackPrev;
  m_OptimumCurrentIndex  = m_Optimum[0].PosPrev;
  return m_OptimumCurrentIndex; 
}

UINT32 CCoder::GetOptimal(UINT32 &backRes)
{
  if(m_OptimumEndIndex != m_OptimumCurrentIndex)
  {
    UINT32 len = m_Optimum[m_OptimumCurrentIndex].PosPrev - m_OptimumCurrentIndex;
    backRes = m_Optimum[m_OptimumCurrentIndex].BackPrev;
    m_OptimumCurrentIndex = m_Optimum[m_OptimumCurrentIndex].PosPrev;
    return len;
  }
  m_OptimumCurrentIndex = 0;
  m_OptimumEndIndex = 0;
  
  GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize));

  UINT32 lenMain = m_LongestMatchLength;
  UINT32 backMain = m_LongestMatchDistance;

  if(lenMain < kMatchMinLen)
    return 1;
  if(lenMain >= m_MatchLengthEdge)
  {
    backRes = backMain; 
    MovePos(lenMain - 1);
    return lenMain;
  }
  m_Optimum[1].Price = m_LiteralPrices[m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset)];
  m_Optimum[1].PosPrev = 0;

  m_Optimum[2].Price = kIfinityPrice;
  m_Optimum[2].PosPrev = 1;

  for(UINT32 i = kMatchMinLen; i <= lenMain; i++)
  {
    m_Optimum[i].PosPrev = 0;
    m_Optimum[i].BackPrev = m_MatchDistances[i];
    m_Optimum[i].Price = m_LenPrices[i - kMatchMinLen] + m_PosPrices[GetPosSlot(m_MatchDistances[i])];
  }


  UINT32 cur = 0;
  UINT32 lenEnd = lenMain;
  while(true)
  {
    cur++;
    if(cur == lenEnd)  
      return Backward(backRes, cur);
    GetBacks(UINT32(m_BlockStartPostion + m_CurrentBlockUncompressedSize + cur));
    UINT32 newLen = m_LongestMatchLength;
    if(newLen >= m_MatchLengthEdge)
      return Backward(backRes, cur);
    
    UINT32 curPrice = m_Optimum[cur].Price; 
    UINT32 curAnd1Price = curPrice +
        m_LiteralPrices[m_MatchFinder.GetIndexByte(cur - m_AdditionalOffset)];
    COptimal &optimum = m_Optimum[cur + 1];
    if (curAnd1Price < optimum.Price) 
    {
      optimum.Price = curAnd1Price;
      optimum.PosPrev = cur;
    }
    if (newLen < kMatchMinLen)
      continue;
    if(cur + newLen > lenEnd)
    {
      if (cur + newLen > kNumOpts - 1)
        newLen = kNumOpts - 1 - cur;
      UINT32 lenEndNew = cur + newLen;
      if (lenEnd < lenEndNew)
      {
        for(UINT32 i = lenEnd + 1; i <= lenEndNew; i++)
          m_Optimum[i].Price = kIfinityPrice;
        lenEnd = lenEndNew;
      }
    }       
    for(UINT32 lenTest = kMatchMinLen; lenTest <= newLen; lenTest++)
    {
      UINT16 curBack = m_MatchDistances[lenTest];
      UINT32 curAndLenPrice = curPrice + 
          m_LenPrices[lenTest - kMatchMinLen] + m_PosPrices[GetPosSlot(curBack)];
      COptimal &optimum = m_Optimum[cur + lenTest];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = cur;
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
 
  UINT32 i;
  for(i = 0; i < 256; i++)
    m_LiteralPrices[i] = 8;
  for(i = 0; i < m_NumLenCombinations; i++)
    m_LenPrices[i] = 5 + m_LenDirectBits[g_LenSlots[i]]; // test it
  for(i = 0; i < kDistTableSize64; i++)
    m_PosPrices[i] = 5 + kDistDirectBits[i];
}

void CCoder::WriteBlockData(bool writeMode, bool finalBlock)
{
  m_MainCoder.AddSymbol(kReadTableNumber);
  int method = WriteTables(writeMode, finalBlock);
  
  if (writeMode)
  {
    if(method == NBlockType::kStored)
    {
      for(UINT32 i = 0; i < m_CurrentBlockUncompressedSize; i++)
      {
        BYTE b = m_MatchFinder.GetIndexByte(i - m_AdditionalOffset - 
              m_CurrentBlockUncompressedSize);
        m_OutStream.WriteBits(b, 8);
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
          UINT32 len = m_Values[i].Len;
          UINT32 lenSlot = g_LenSlots[len];
          m_MainCoder.CodeOneValue(&m_ReverseOutStream, kMatchNumber + lenSlot);
          m_OutStream.WriteBits(len - m_LenStart[lenSlot], m_LenDirectBits[lenSlot]);
          UINT32 dist = m_Values[i].Pos;
          UINT32 posSlot = GetPosSlot(dist);
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
  UINT32 i;
  for(i = 0; i < 256; i++)
    if(m_LastLevels[i] != 0)
      m_LiteralPrices[i] = m_LastLevels[i];
    else
      m_LiteralPrices[i] = kNoLiteralDummy;

  // -------------- Normal match -----------------------------
  
  for(i = 0; i < m_NumLenCombinations; i++)
  {
    UINT32 slot = g_LenSlots[i];
    BYTE dummy = m_LastLevels[kMatchNumber + slot];
    if (dummy != 0)
      m_LenPrices[i] = dummy;
    else
      m_LenPrices[i] = kNoLenDummy;
    m_LenPrices[i] += m_LenDirectBits[slot];
  }
  for(i = 0; i < kDistTableSize64; i++)
  {
    BYTE dummy = m_LastLevels[kDistTableStart + i];
    if (dummy != 0)
      m_PosPrices[i] = dummy;
    else
      m_PosPrices[i] = kNoPosDummy;
    m_PosPrices[i] += kDistDirectBits[i];
  }
}

void CCoder::CodeLevelTable(BYTE *newLevels, int numLevels, bool codeMode)
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
  BYTE oldValueInGuardElement = newLevels[numLevels]; // push guard value
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
  BYTE newLevels[kMaxTableSize64 + 1]; // (+ 1) for guard 

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
    

    BYTE levelLevels[kLevelTableSize];
    m_LevelCoder.BuildTree(levelLevels);
    
    BYTE levelLevelsStream[kLevelTableSize];
    int numLevelCodes = kDeflateNumberOfLevelCodesMin;
    int i;
    for (i = 0; i < kLevelTableSize; i++)
    {
      int streamPos = kCodeLengthAlphabetOrder[i];
      int level = levelLevels[streamPos]; 
      if (level > 0 && i >= numLevelCodes)
        numLevelCodes = i + 1;
      levelLevelsStream[i] = level;
    }
    
    UINT32 numLZHuffmanBits = m_MainCoder.GetBlockBitLength();
    numLZHuffmanBits += m_DistCoder.GetBlockBitLength();
    numLZHuffmanBits += m_LevelCoder.GetBlockBitLength();
    numLZHuffmanBits += kDeflateNumberOfLengthCodesFieldSize +
      kDeflateNumberOfDistanceCodesFieldSize +
      kDeflateNumberOfLevelCodesFieldSize;
    numLZHuffmanBits += numLevelCodes * kDeflateLevelCodeFieldSize;

    UINT32 nextBitPosition = 
        (m_OutStream.GetBitPosition() + kBlockTypeFieldSize) % 8;
    UINT32 numBitsForAlign = nextBitPosition > 0 ? (8 - nextBitPosition): 0;

    UINT32 numStoreBits = numBitsForAlign + (2 * sizeof(UINT16)) * 8;
    numStoreBits += m_CurrentBlockUncompressedSize * 8;
    if(numStoreBits < numLZHuffmanBits)
    {
      m_OutStream.WriteBits(NBlockType::kStored, kBlockTypeFieldSize); // test it
      m_OutStream.WriteBits(0, numBitsForAlign); // test it
      UINT16 currentBlockUncompressedSize = UINT16(m_CurrentBlockUncompressedSize);
      UINT16 currentBlockUncompressedSizeNot = ~currentBlockUncompressedSize;
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
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!m_Created)
  {
    RETURN_IF_NOT_S_OK(Create());
    m_Created = true;
  }

  UINT64 nowPos = 0;
  m_FinderPos = 0;

  RETURN_IF_NOT_S_OK(m_MatchFinder.Init(inStream));
  m_OutStream.Init(outStream);
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
        noMoreBytes = (m_AdditionalOffset == 0 && m_MatchFinder.GetNumAvailableBytes() == 0);
  
        if (((m_CurrentBlockUncompressedSize >= kBlockUncompressedSizeThreshold || 
                 m_ValueIndex >= kValueBlockSize) && 
              (m_OptimumEndIndex == m_OptimumCurrentIndex)) 
            || noMoreBytes)
          break;
        UINT32 pos;
        UINT32 len = GetOptimal(pos);
        if (len >= kMatchMinLen)
        {
          UINT32 newLen = len - kMatchMinLen;
          m_Values[m_ValueIndex].Flag = kFlagLenPos;
          m_Values[m_ValueIndex].Len = BYTE(newLen);
          UINT32 lenSlot = g_LenSlots[newLen];
          m_MainCoder.AddSymbol(kMatchNumber + lenSlot);
          m_Values[m_ValueIndex].Pos = UINT16(pos);
          UINT32 posSlot = GetPosSlot(pos);
          m_DistCoder.AddSymbol(posSlot);
        }
        else if (len == 1)  
        {
          BYTE b = m_MatchFinder.GetIndexByte(0 - m_AdditionalOffset);
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
      m_AdditionalOffset = UINT32(m_FinderPos - m_BlockStartPostion);
      m_CurrentBlockUncompressedSize = 0;
    }
    m_BlockStartPostion += m_CurrentBlockUncompressedSize;
    m_CurrentBlockUncompressedSize = 0;
    if (progress != NULL)
    {
      UINT64 packSize = m_OutStream.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos, &packSize));
    }
    if (noMoreBytes)
      break;
  }
  return  m_OutStream.Flush();
}

HRESULT CCoder::BaseCode(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
  }
  catch(CMatchFinderException &outWriteException)
  {
    return outWriteException.m_Result;
  }
  catch(const NStream::COutByteWriteException &exception)
  {
    return exception.Result;
  }
  catch(...)
  {
    return E_FAIL;
  }
}

STDMETHODIMP CCOMCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

STDMETHODIMP CCOMCoder::SetEncoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
  { return BaseSetEncoderProperties2(propIDs, properties, numProperties); }

STDMETHODIMP CCOMCoder64::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
  { return BaseCode(inStream, outStream, inSize, outSize, progress); }

STDMETHODIMP CCOMCoder64::SetEncoderProperties2(const PROPID *propIDs, 
    const PROPVARIANT *properties, UINT32 numProperties)
  { return BaseSetEncoderProperties2(propIDs, properties, numProperties); }

}}
