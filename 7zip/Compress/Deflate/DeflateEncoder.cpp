// DeflateEncoder.cpp

#include "StdAfx.h"

#include "DeflateEncoder.h"

#include "Windows/Defs.h"
#include "Common/ComTry.h"
#include "../../../Common/Alloc.h"
#include "../LZ/BinTree/BinTree3Z.h"

namespace NCompress {
namespace NDeflate {
namespace NEncoder {

const int kNumDivPassesMax = 10; // [0, 16); ratio/speed/ram tradeoff; use big value for better compression ratio.
const UInt32 kNumTables = (1 << kNumDivPassesMax);

static UInt32 kFixedHuffmanCodeBlockSizeMax = (1 << 8); // [0, (1 << 32)); ratio/speed tradeoff; use big value for better compression ratio.
static UInt32 kDivideCodeBlockSizeMin = (1 << 6); // [1, (1 << 32)); ratio/speed tradeoff; use small value for better compression ratio.
static UInt32 kDivideBlockSizeMin = (1 << 6); // [1, (1 << 32)); ratio/speed tradeoff; use small value for better compression ratio.

static const UInt32 kMaxUncompressedBlockSize = ((1 << 16) - 1) * 1; // [1, (1 << 32))
static const UInt32 kMatchArraySize = kMaxUncompressedBlockSize * 10; // [kMatchMaxLen * 2, (1 << 32))
static const UInt32 kMatchArrayLimit = kMatchArraySize - kMatchMaxLen * 4 * sizeof(UInt16);
static const UInt32 kBlockUncompressedSizeThreshold = kMaxUncompressedBlockSize - 
    kMatchMaxLen - kNumOpts;

static const int kMaxCodeBitLength = 15;
static const int kMaxLevelBitLength = 7;

struct CMatchFinderException
{
  HRESULT ErrorCode;
  CMatchFinderException(HRESULT errorCode): ErrorCode(errorCode) {}
};

static Byte kNoLiteralStatPrice = 13;
static Byte kNoLenStatPrice = 13;
static Byte kNoPosStatPrice = 6;

static Byte g_LenSlots[kNumLenSymbolsMax];
static Byte g_FastPos[1 << 9];

class CFastPosInit
{
public:
  CFastPosInit()
  {
    int i;
    for(i = 0; i < kNumLenSlots; i++)
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
  if (pos < 0x200)
    return g_FastPos[pos];
  return g_FastPos[pos >> 8] + 16;
}

CCoder::CCoder(bool deflate64Mode):
  m_Deflate64Mode(deflate64Mode),
  m_NumPasses(1),
  m_NumDivPasses(1),
  m_NumFastBytes(32),
  m_OnePosMatchesMemory(0),
  m_DistanceMemory(0),
  m_Created(false),
  m_Values(0),
  m_Tables(0)
{
  m_MatchMaxLen = deflate64Mode ? kMatchMaxLen64 : kMatchMaxLen32;
  m_NumLenCombinations = deflate64Mode ? kNumLenSymbols64 : kNumLenSymbols32;
  m_LenStart = deflate64Mode ? kLenStart64 : kLenStart32;
  m_LenDirectBits = deflate64Mode ? kLenDirectBits64 : kLenDirectBits32;
}

HRESULT CCoder::Create()
{
  COM_TRY_BEGIN
  if (!m_MatchFinder)
  {
    m_MatchFinder = new NBT3Z::CMatchFinder;
    if (m_MatchFinder == 0)
      return E_OUTOFMEMORY;
  }
  if (m_Values == 0)
  {
    m_Values = (CCodeValue *)MyAlloc((kMaxUncompressedBlockSize) * sizeof(CCodeValue));
    if (m_Values == 0)
      return E_OUTOFMEMORY;
  }
  if (m_Tables == 0)
  {
    m_Tables = (CTables *)MyAlloc((kNumTables) * sizeof(CTables));
    if (m_Tables == 0)
      return E_OUTOFMEMORY;
  }

  if (m_IsMultiPass)
  {
    if (m_OnePosMatchesMemory == 0)
    {
      m_OnePosMatchesMemory = (UInt16 *)::MidAlloc(kMatchArraySize * sizeof(UInt16));
      if (m_OnePosMatchesMemory == 0)
        return E_OUTOFMEMORY;
    }
  }
  else
  {
    if (m_DistanceMemory == 0)
    {
      m_DistanceMemory = (UInt16 *)MyAlloc((kMatchMaxLen + 2) * 2 * sizeof(UInt16));
      if (m_DistanceMemory == 0)
        return E_OUTOFMEMORY;
      m_MatchDistances = m_DistanceMemory;
    }
  }

  if (!m_Created)
  {
    RINOK(m_MatchFinder->Create(m_Deflate64Mode ? kHistorySize64 : kHistorySize32, 
      kNumOpts + kMaxUncompressedBlockSize, m_NumFastBytes, m_MatchMaxLen - m_NumFastBytes));
    if (!m_OutStream.Create(1 << 20))
      return E_OUTOFMEMORY;
    if (!MainCoder.Create(kFixedMainTableSize, m_LenDirectBits, kSymbolMatch, kMaxCodeBitLength))
      return E_OUTOFMEMORY;
    if (!DistCoder.Create(kDistTableSize64, kDistDirectBits, 0, kMaxCodeBitLength))
      return E_OUTOFMEMORY;
    if (!LevelCoder.Create(kLevelTableSize, kLevelDirectBits, kTableDirectLevels, kMaxLevelBitLength))
      return E_OUTOFMEMORY;
  }
  m_Created = true;
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
        m_NumDivPasses = property.ulVal;
        if (m_NumDivPasses == 0)
          m_NumDivPasses = 1;
        if (m_NumDivPasses == 1)
          m_NumPasses = 1;
        else if (m_NumDivPasses <= kNumDivPassesMax)
          m_NumPasses = 2;
        else
        {
          m_NumPasses = 2 + (m_NumDivPasses - kNumDivPassesMax);
          m_NumDivPasses = kNumDivPassesMax;
        }
        break;
      case NCoderPropID::kNumFastBytes:
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        m_NumFastBytes = property.ulVal;
        if(m_NumFastBytes < kMatchMinLen || m_NumFastBytes > m_MatchMaxLen)
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
  ::MidFree(m_OnePosMatchesMemory);
  m_OnePosMatchesMemory = 0;
  ::MyFree(m_DistanceMemory);
  m_DistanceMemory = 0;
  ::MyFree(m_Values);
  m_Values = 0;
  ::MyFree(m_Tables);
  m_Tables = 0;
}

CCoder::~CCoder()
{
  Free();
}

void CCoder::GetMatches()
{
  if (m_IsMultiPass)
  {
    m_MatchDistances = m_OnePosMatchesMemory + m_Pos;
    if (m_SecondPass)
    {
      m_Pos += *m_MatchDistances + 1;
      return;
    }
  }

  UInt32 distanceTmp[kMatchMaxLen * 2 + 3];
  
  HRESULT result = m_MatchFinder->GetMatches(distanceTmp);
  if (result != S_OK)
    throw CMatchFinderException(result);
  UInt32 numPairs = distanceTmp[0];

  *m_MatchDistances = (UInt16)numPairs;
   
  if (numPairs > 0)
  {
    UInt32 i = 1;
    for(i = 1; i < numPairs; i += 2)
    {
      m_MatchDistances[i] = (UInt16)distanceTmp[i];
      m_MatchDistances[i + 1] = (UInt16)distanceTmp[i + 1];
    }
    UInt32 len = distanceTmp[1 + numPairs - 2];
    if (len == m_NumFastBytes && m_NumFastBytes != m_MatchMaxLen)
      m_MatchDistances[i - 2] = (UInt16)(len + m_MatchFinder->GetMatchLen(len - 1, 
      distanceTmp[1 + numPairs - 1], m_MatchMaxLen - len));
  }
  if (m_IsMultiPass)
    m_Pos += numPairs + 1;
  if (!m_SecondPass)
    m_AdditionalOffset++;
}

void CCoder::MovePos(UInt32 num)
{
  if (!m_SecondPass && num > 0)
  {
    HRESULT result = m_MatchFinder->Skip(num);
    if (result != S_OK)
      throw CMatchFinderException(result);
    m_AdditionalOffset += num;
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
  m_OptimumCurrentIndex = m_Optimum[0].PosPrev;
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
  m_OptimumCurrentIndex = m_OptimumEndIndex = 0;
  
  GetMatches();

  UInt32 numDistancePairs = m_MatchDistances[0];
  if(numDistancePairs == 0)
    return 1;

  const UInt16 *matchDistances = m_MatchDistances + 1;
  UInt32 lenMain = matchDistances[numDistancePairs - 2];

  if(lenMain > m_NumFastBytes)
  {
    backRes = matchDistances[numDistancePairs - 1]; 
    MovePos(lenMain - 1);
    return lenMain;
  }
  m_Optimum[1].Price = m_LiteralPrices[m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset)];
  m_Optimum[1].PosPrev = 0;

  m_Optimum[2].Price = kIfinityPrice;
  m_Optimum[2].PosPrev = 1;


  UInt32 offs = 0;
  for(UInt32 i = kMatchMinLen; i <= lenMain; i++)
  {
    UInt32 distance = matchDistances[offs + 1];
    m_Optimum[i].PosPrev = 0;
    m_Optimum[i].BackPrev = distance;
    m_Optimum[i].Price = m_LenPrices[i - kMatchMinLen] + m_PosPrices[GetPosSlot(distance)];
    if (i == matchDistances[offs])
      offs += 2;
  }

  UInt32 cur = 0;
  UInt32 lenEnd = lenMain;
  while(true)
  {
    ++cur;
    if(cur == lenEnd || cur == kNumOptsBase || m_Pos >= kMatchArrayLimit)  
      return Backward(backRes, cur);
    GetMatches();
    matchDistances = m_MatchDistances + 1;

    UInt32 numDistancePairs = m_MatchDistances[0];
    UInt32 newLen = 0;
    if(numDistancePairs != 0)
    { 
      newLen = matchDistances[numDistancePairs - 2];
      if(newLen > m_NumFastBytes)
      {
        UInt32 len = Backward(backRes, cur);
        m_Optimum[cur].BackPrev = matchDistances[numDistancePairs - 1];
        m_Optimum[cur].PosPrev = m_OptimumEndIndex = cur + newLen;
        MovePos(newLen - 1);
        return len;
      }
    }
    UInt32 curPrice = m_Optimum[cur].Price; 
    UInt32 curAnd1Price = curPrice + m_LiteralPrices[m_MatchFinder->GetIndexByte(cur - m_AdditionalOffset)];
    COptimal &optimum = m_Optimum[cur + 1];
    if (curAnd1Price < optimum.Price) 
    {
      optimum.Price = curAnd1Price;
      optimum.PosPrev = (UInt16)cur;
    }
    if(numDistancePairs == 0)
      continue;
    while(lenEnd < cur + newLen)
      m_Optimum[++lenEnd].Price = kIfinityPrice;
    offs = 0;
    UInt32 distance = matchDistances[offs + 1];
    curPrice += m_PosPrices[GetPosSlot(distance)];
    for(UInt32 lenTest = kMatchMinLen; ; lenTest++)
    {
      UInt32 curAndLenPrice = curPrice + m_LenPrices[lenTest - kMatchMinLen];
      COptimal &optimum = m_Optimum[cur + lenTest];
      if (curAndLenPrice < optimum.Price) 
      {
        optimum.Price = curAndLenPrice;
        optimum.PosPrev = (UInt16)cur;
        optimum.BackPrev = distance;
      }
      if (lenTest == matchDistances[offs])
      {
        offs += 2;
        if (offs == numDistancePairs)
          break;
        curPrice -= m_PosPrices[GetPosSlot(distance)];
        distance = matchDistances[offs + 1];
        curPrice += m_PosPrices[GetPosSlot(distance)];
      }
    }
  }
}

void CTables::InitStructures()
{
  UInt32 i;
  for(i = 0; i < 256; i++)
    litLenLevels[i] = 8;
  litLenLevels[i++] = 13;
  for(;i < kFixedMainTableSize; i++)
    litLenLevels[i] = 5;
  for(i = 0; i < kFixedDistTableSize; i++)
    distLevels[i] = 5;
}

void CCoder::CodeLevelTable(NStream::NLSBF::CEncoder *outStream, const Byte *levels, int numLevels)
{
  int prevLen = 0xFF;
  int nextLen = levels[0];
  int count = 0;
  int maxCount = 7;
  int minCount = 4;
  if (nextLen == 0) 
  {
    maxCount = 138;
    minCount = 3;
  }
  for (int n = 0; n < numLevels; n++) 
  {
    int curLen = nextLen; 
    nextLen = (n < numLevels - 1) ? levels[n + 1] : 0xFF;
    count++;
    if (count < maxCount && curLen == nextLen) 
      continue;
    
    if (count < minCount) 
      for(int i = 0; i < count; i++) 
        if (outStream != 0)
          LevelCoder.CodeOneValue(outStream, curLen);
        else
          LevelCoder.AddSymbol(curLen);
    else if (curLen != 0) 
    {
      if (curLen != prevLen) 
      {
        if (outStream != 0)
          LevelCoder.CodeOneValue(outStream, curLen);
        else
          LevelCoder.AddSymbol(curLen);
        count--;
      }
      if (outStream != 0)
      {
        LevelCoder.CodeOneValue(outStream, kTableLevelRepNumber);
        outStream->WriteBits(count - 3, 2);
      }
      else
        LevelCoder.AddSymbol(kTableLevelRepNumber);
    } 
    else if (count <= 10) 
      if (outStream != 0)
      {
        LevelCoder.CodeOneValue(outStream, kTableLevel0Number);
        outStream->WriteBits(count - 3, 3);
      }
      else
        LevelCoder.AddSymbol(kTableLevel0Number);
    else 
      if (outStream != 0)
      {
        LevelCoder.CodeOneValue(outStream, kTableLevel0Number2);
        outStream->WriteBits(count - 11, 7);
      }
      else
        LevelCoder.AddSymbol(kTableLevel0Number2);

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

void CCoder::MakeTables()
{
  MainCoder.BuildTree(m_NewLevels.litLenLevels);
  DistCoder.BuildTree(m_NewLevels.distLevels);
  MainCoder.ReverseBits();
  DistCoder.ReverseBits();
}

UInt32 CCoder::GetLzBlockPrice()
{
  LevelCoder.StartNewBlock();
  
  m_NumLitLenLevels = kMainTableSize;
  while(m_NumLitLenLevels > kNumLitLenCodesMin && m_NewLevels.litLenLevels[m_NumLitLenLevels - 1] == 0)
    m_NumLitLenLevels--;
  
  m_NumDistLevels = kDistTableSize64;
  while(m_NumDistLevels > kNumDistCodesMin && m_NewLevels.distLevels[m_NumDistLevels - 1] == 0)
    m_NumDistLevels--;
  
  CodeLevelTable(0, m_NewLevels.litLenLevels, m_NumLitLenLevels);
  CodeLevelTable(0, m_NewLevels.distLevels, m_NumDistLevels);
  
  Byte levelLevels[kLevelTableSize];
  LevelCoder.BuildTree(levelLevels);
  LevelCoder.ReverseBits();
  
  m_NumLevelCodes = kNumLevelCodesMin;
  for (UInt32 i = 0; i < kLevelTableSize; i++)
  {
    Byte level = levelLevels[kCodeLengthAlphabetOrder[i]]; 
    if (level > 0 && i >= m_NumLevelCodes)
      m_NumLevelCodes = i + 1;
    m_LevelLevels[i] = level;
  }
  
  return MainCoder.GetBlockBitLength() + DistCoder.GetBlockBitLength() + LevelCoder.GetBlockBitLength() +
      kNumLenCodesFieldSize + kNumDistCodesFieldSize + kNumLevelCodesFieldSize +
      m_NumLevelCodes * kLevelFieldSize + kFinalBlockFieldSize + kBlockTypeFieldSize;
}

void CCoder::TryBlock(bool staticMode)
{
  MainCoder.StartNewBlock();
  DistCoder.StartNewBlock();
  m_ValueIndex = 0;
  UInt32 blockSize = BlockSizeRes;
  BlockSizeRes = 0;
  while(true)
  {
    if (m_OptimumCurrentIndex == m_OptimumEndIndex)
    {
      if (m_Pos >= kMatchArrayLimit || BlockSizeRes >= blockSize || !m_SecondPass && 
          ((m_MatchFinder->GetNumAvailableBytes() == 0) || m_ValueIndex >= m_ValueBlockSize))
        break;
    }
    UInt32 pos;
    UInt32 len = GetOptimal(pos);
    CCodeValue &codeValue = m_Values[m_ValueIndex++];
    if (len >= kMatchMinLen)
    {
      UInt32 newLen = len - kMatchMinLen;
      codeValue.Len = (UInt16)newLen;
      MainCoder.AddSymbol(kSymbolMatch + g_LenSlots[newLen]);
      codeValue.Pos = (UInt16)pos;
      DistCoder.AddSymbol(GetPosSlot(pos));
    }
    else
    {
      Byte b = m_MatchFinder->GetIndexByte(0 - m_AdditionalOffset);
      MainCoder.AddSymbol(b);
      codeValue.SetAsLiteral();
      codeValue.Pos = b;
    }
    m_AdditionalOffset -= len;
    BlockSizeRes += len;
  }
  MainCoder.AddSymbol(kSymbolEndOfBlock);
  if (!staticMode)
  {
    MakeTables();
    SetPrices(m_NewLevels);
  }
  m_AdditionalOffset += BlockSizeRes;
  m_SecondPass = true;
}

void CCoder::SetPrices(const CLevels &levels)
{
  UInt32 i;
  for(i = 0; i < 256; i++)
  {
    Byte price = levels.litLenLevels[i];
    m_LiteralPrices[i] = ((price != 0) ? price : kNoLiteralStatPrice);
  }
  
  for(i = 0; i < m_NumLenCombinations; i++)
  {
    UInt32 slot = g_LenSlots[i];
    Byte price = levels.litLenLevels[kSymbolMatch + slot];
    m_LenPrices[i] = ((price != 0) ? price : kNoLenStatPrice) + m_LenDirectBits[slot];
  }
  
  for(i = 0; i < kDistTableSize64; i++)
  {
    Byte price = levels.distLevels[i];
    m_PosPrices[i] = ((price != 0) ? price: kNoPosStatPrice) + kDistDirectBits[i];
  }
}

void CCoder::WriteBlock()
{
  for (UInt32 i = 0; i < m_ValueIndex; i++)
  {
    const CCodeValue &codeValue = m_Values[i];
    if (codeValue.IsLiteral())
      MainCoder.CodeOneValue(&m_OutStream, codeValue.Pos);
    else
    {
      UInt32 len = codeValue.Len;
      UInt32 lenSlot = g_LenSlots[len];
      MainCoder.CodeOneValue(&m_OutStream, kSymbolMatch + lenSlot);
      m_OutStream.WriteBits(len - m_LenStart[lenSlot], m_LenDirectBits[lenSlot]);
      UInt32 dist = codeValue.Pos;
      UInt32 posSlot = GetPosSlot(dist);
      DistCoder.CodeOneValue(&m_OutStream, posSlot);
      m_OutStream.WriteBits(dist - kDistStart[posSlot], kDistDirectBits[posSlot]);
    }
  }
  MainCoder.CodeOneValue(&m_OutStream, kSymbolEndOfBlock);
}

void CCoder::WriteDynBlock(bool finalBlock)
{
  m_OutStream.WriteBits((finalBlock ? NFinalBlockField::kFinalBlock: NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
  m_OutStream.WriteBits(NBlockType::kDynamicHuffman, kBlockTypeFieldSize);
  m_OutStream.WriteBits(m_NumLitLenLevels - kNumLitLenCodesMin, kNumLenCodesFieldSize);
  m_OutStream.WriteBits(m_NumDistLevels - kNumDistCodesMin, kNumDistCodesFieldSize);
  m_OutStream.WriteBits(m_NumLevelCodes - kNumLevelCodesMin, kNumLevelCodesFieldSize);
      
  for (UInt32 i = 0; i < m_NumLevelCodes; i++)
    m_OutStream.WriteBits(m_LevelLevels[i], kLevelFieldSize);
      
  CodeLevelTable(&m_OutStream, m_NewLevels.litLenLevels, m_NumLitLenLevels);
  CodeLevelTable(&m_OutStream, m_NewLevels.distLevels, m_NumDistLevels);

  WriteBlock();
}

void CCoder::WriteFixedBlock(bool finalBlock)
{
  int i;
  for (i = 0; i < kFixedMainTableSize; i++)
    MainCoder.SetFreq(i, (UInt32)1 << (kNumHuffmanBits - m_NewLevels.litLenLevels[i]));
  for (i = 0; i < kFixedDistTableSize; i++)
    DistCoder.SetFreq(i, (UInt32)1 << (kNumHuffmanBits - m_NewLevels.distLevels[i]));
  MakeTables();

  m_OutStream.WriteBits((finalBlock ? NFinalBlockField::kFinalBlock: NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
  m_OutStream.WriteBits(NBlockType::kFixedHuffman, kBlockTypeFieldSize);
  WriteBlock();
}

static UInt32 GetStorePrice(UInt32 blockSize, int bitPosition)
{
  UInt32 price = 0;
  do
  {
    UInt32 nextBitPosition = (bitPosition + kFinalBlockFieldSize + kBlockTypeFieldSize) & 7;
    int numBitsForAlign = nextBitPosition > 0 ? (8 - nextBitPosition): 0;
    UInt32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
    price += kFinalBlockFieldSize + kBlockTypeFieldSize + numBitsForAlign + (2 + 2) * 8 + curBlockSize * 8;
    bitPosition = 0;
    blockSize -= curBlockSize;
  }
  while(blockSize != 0);
  return price;
}

void CCoder::WriteStoreBlock(UInt32 blockSize, UInt32 additionalOffset, bool finalBlock)
{
  do
  {
    UInt32 curBlockSize = (blockSize < (1 << 16)) ? blockSize : (1 << 16) - 1;
    blockSize -= curBlockSize;
    m_OutStream.WriteBits((finalBlock && (blockSize == 0) ? NFinalBlockField::kFinalBlock: NFinalBlockField::kNotFinalBlock), kFinalBlockFieldSize);
    m_OutStream.WriteBits(NBlockType::kStored, kBlockTypeFieldSize);
    m_OutStream.FlushByte();
    m_OutStream.WriteBits((UInt16)curBlockSize, kStoredBlockLengthFieldSize);
    m_OutStream.WriteBits((UInt16)~curBlockSize, kStoredBlockLengthFieldSize);
    const Byte *data = m_MatchFinder->GetPointerToCurrentPos() - additionalOffset;
    for(UInt32 i = 0; i < curBlockSize; i++)
      m_OutStream.WriteByte(data[i]);
    additionalOffset -= curBlockSize;
  }
  while(blockSize != 0);
}

UInt32 CCoder::TryDynBlock(int tableIndex, UInt32 numPasses)
{
  CTables &t = m_Tables[tableIndex];
  BlockSizeRes = t.BlockSizeRes;
  m_Pos = t.m_Pos;
  SetPrices(t);

  for (UInt32 p = 0; p < numPasses; p++)
  {
    UInt32 posTemp = m_Pos;
    TryBlock(false);
    if (p != numPasses - 1)
      m_Pos = posTemp;
  }
  const UInt32 lzPrice = GetLzBlockPrice();
  (CLevels &)t = m_NewLevels;
  return lzPrice;
}

UInt32 CCoder::TryFixedBlock(int tableIndex)
{
  CTables &t = m_Tables[tableIndex];
  BlockSizeRes = t.BlockSizeRes;
  m_Pos = t.m_Pos;
  m_NewLevels.SetFixedLevels();
  SetPrices(m_NewLevels);

  TryBlock(true);
  return kFinalBlockFieldSize + kBlockTypeFieldSize +
      MainCoder.GetPrice(m_NewLevels.litLenLevels) + 
      DistCoder.GetPrice(m_NewLevels.distLevels);
}

UInt32 CCoder::GetBlockPrice(int tableIndex, int numDivPasses)
{
  CTables &t = m_Tables[tableIndex];
  t.StaticMode = false;
  UInt32 price = TryDynBlock(tableIndex, m_NumPasses);
  t.BlockSizeRes = BlockSizeRes;
  UInt32 numValues = m_ValueIndex;
  UInt32 posTemp = m_Pos;
  UInt32 additionalOffsetEnd = m_AdditionalOffset;
  
  if (m_CheckStatic && m_ValueIndex <= kFixedHuffmanCodeBlockSizeMax)
  {
    const UInt32 fixedPrice = TryFixedBlock(tableIndex);
    if (t.StaticMode = (fixedPrice < price))
      price = fixedPrice;
  }

  const UInt32 storePrice = GetStorePrice(BlockSizeRes, 0); // bitPosition
  if (t.StoreMode = (storePrice <= price))
    price = storePrice;

  t.UseSubBlocks = false;

  if (numDivPasses > 1 && numValues >= kDivideCodeBlockSizeMin)
  {
    CTables &t0 = m_Tables[(tableIndex << 1)];
    (CLevels &)t0 = t;
    t0.BlockSizeRes = t.BlockSizeRes >> 1;
    t0.m_Pos = t.m_Pos;
    UInt32 subPrice = GetBlockPrice((tableIndex << 1), numDivPasses - 1);

    UInt32 blockSize2 = t.BlockSizeRes - t0.BlockSizeRes;
    if (t0.BlockSizeRes >= kDivideBlockSizeMin && blockSize2 >= kDivideBlockSizeMin)
    {
      CTables &t1 = m_Tables[(tableIndex << 1) + 1];
      (CLevels &)t1 = t;
      t1.BlockSizeRes = blockSize2;
      t1.m_Pos = m_Pos;
      m_AdditionalOffset -= t0.BlockSizeRes;
      subPrice += GetBlockPrice((tableIndex << 1) + 1, numDivPasses - 1);
      if (t.UseSubBlocks = (subPrice < price))
        price = subPrice;
    }
  }
  m_AdditionalOffset = additionalOffsetEnd;
  m_Pos = posTemp;
  return price;
}

void CCoder::CodeBlock(int tableIndex, bool finalBlock)
{
  CTables &t = m_Tables[tableIndex];
  if (t.UseSubBlocks)
  {
    CodeBlock((tableIndex << 1), false);
    CodeBlock((tableIndex << 1) + 1, finalBlock);
  }
  else
  {
    if (t.StoreMode)
      WriteStoreBlock(t.BlockSizeRes, m_AdditionalOffset, finalBlock);
    else
      if (t.StaticMode)
      {
        TryFixedBlock(tableIndex);
        WriteFixedBlock(finalBlock);
      }
      else 
      {
        if (m_NumDivPasses > 1 || m_CheckStatic)
          TryDynBlock(tableIndex, 1);
        WriteDynBlock(finalBlock);
      }
    m_AdditionalOffset -= t.BlockSizeRes;
  }
}

HRESULT CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  m_CheckStatic = (m_NumPasses != 1 || m_NumDivPasses != 1);
  m_IsMultiPass = (m_CheckStatic || (m_NumPasses != 1 || m_NumDivPasses != 1));

  RINOK(Create());

  m_ValueBlockSize = (1 << 13) + (1 << 12) * m_NumDivPasses;

  UInt64 nowPos = 0;

  RINOK(m_MatchFinder->SetStream(inStream));
  RINOK(m_MatchFinder->Init());
  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  CCoderReleaser coderReleaser(this);

  m_OptimumEndIndex = m_OptimumCurrentIndex = 0;

  CTables &t = m_Tables[1];
  t.m_Pos = 0;
  t.InitStructures();

  m_AdditionalOffset = 0;
  do
  {
    t.BlockSizeRes = kBlockUncompressedSizeThreshold;
    m_SecondPass = false;
    GetBlockPrice(1, m_NumDivPasses);
    CodeBlock(1, m_MatchFinder->GetNumAvailableBytes() == 0);
    nowPos += m_Tables[1].BlockSizeRes;
    if (progress != NULL)
    {
      UInt64 packSize = m_OutStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&nowPos, &packSize));
    }
  }
  while(m_MatchFinder->GetNumAvailableBytes() != 0);
  return m_OutStream.Flush();
}

HRESULT CCoder::BaseCode(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(CMatchFinderException &e) { return e.ErrorCode; }
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

