// BZip2Encoder.cpp

#include "StdAfx.h"

#include "BZip2Encoder.h"

#include "../../../Common/Alloc.h"
#include "../BWT/Mtf8.h"
#include "BZip2CRC.h"

namespace NCompress {
namespace NBZip2 {

static const UInt32 kBufferSize = (1 << 17);
static const int kNumHuffPasses = 4;

CEncoder::CEncoder(): 
  m_Block(0), 
  m_NeedHuffmanCreate(true), 
  m_NumPasses(1), 
  m_OptimizeNumTables(false),
  m_BlockSizeMult(kBlockSizeMultMax)
{}

CEncoder::~CEncoder()
{
  ::BigFree(m_Block);
}

UInt32 CEncoder::ReadRleBlock(Byte *buffer)
{
  UInt32 i = 0;
  Byte prevByte;
  if (m_InStream.ReadByte(prevByte))
  {
    UInt32 blockSize = m_BlockSizeMult * kBlockSizeStep - 1;
    int numReps = 1;
    buffer[i++] = prevByte;
    while (i < blockSize) // "- 1" to support RLE
    {
      Byte b;
      if (!m_InStream.ReadByte(b))
        break;
      if (b != prevByte)
      {
        if (numReps >= kRleModeRepSize)
          buffer[i++] = numReps - kRleModeRepSize;
        buffer[i++] = b;
        numReps = 1;
        prevByte = b;
        continue;
      }
      numReps++;
      if (numReps <= kRleModeRepSize)
        buffer[i++] = b;
      else if (numReps == kRleModeRepSize + 255)
      {
        buffer[i++] = numReps - kRleModeRepSize;
        numReps = 0;
      }
    }
    // it's to support original BZip2 decoder
    if (numReps >= kRleModeRepSize)
      buffer[i++] = numReps - kRleModeRepSize;
  }
  return i;
}

void CEncoder::WriteBits2(UInt32 value, UInt32 numBits)
  { m_OutStreamCurrent->WriteBits(value, numBits); }
void CEncoder::WriteByte2(Byte b) { WriteBits2(b , 8); }
void CEncoder::WriteBit2(bool v) { WriteBits2((v ? 1 : 0), 1); }
void CEncoder::WriteCRC2(UInt32 v) 
{ 
  for (int i = 0; i < 4; i++)
    WriteByte2(((Byte)(v >> (24 - i * 8))));
}

void CEncoder::WriteBits(UInt32 value, UInt32 numBits)
  { m_OutStream.WriteBits(value, numBits); }
void CEncoder::WriteByte(Byte b) { WriteBits(b , 8); }
void CEncoder::WriteBit(bool v) { WriteBits((v ? 1 : 0), 1); }
void CEncoder::WriteCRC(UInt32 v) 
{ 
  for (int i = 0; i < 4; i++)
    WriteByte(((Byte)(v >> (24 - i * 8))));
}


// blockSize > 0
void CEncoder::EncodeBlock(Byte *block, UInt32 blockSize)
{
  WriteBit2(false); // Randomised = false
  
  {
    UInt32 origPtr = m_BlockSorter.Sort(block, blockSize);
    WriteBits2(origPtr, kNumOrigBits);
  }

  CMtf8Encoder mtf;
  int numInUse = 0;
  {
    bool inUse[256];
    bool inUse16[16];
    UInt32 i;
    for (i = 0; i < 256; i++) 
      inUse[i] = false;
    for (i = 0; i < 16; i++) 
      inUse16[i] = false;
    for (i = 0; i < blockSize; i++)
      inUse[block[i]] = true;
    for (i = 0; i < 256; i++) 
      if (inUse[i])
      {
        inUse16[i >> 4] = true;
        mtf.Buffer[numInUse++] = (Byte)i;
      }
    for (i = 0; i < 16; i++) 
      WriteBit2(inUse16[i]);
    for (i = 0; i < 256; i++) 
      if (inUse16[i >> 4])
        WriteBit2(inUse[i]);
  }
  int alphaSize = numInUse + 2;

  Byte *mtfs = m_MtfArray;
  UInt32 mtfArraySize = 0;
  UInt32 symbolCounts[kMaxAlphaSize];
  {
    for (int i = 0; i < kMaxAlphaSize; i++)
      symbolCounts[i] = 0;
  }

  {
    UInt32 rleSize = 0;
    UInt32 i = 0;
    do 
    {
      UInt32 index = m_BlockSorter.Indices[i];
      if (index == 0)
        index = blockSize - 1;
      else
        index--;
      int pos = mtf.FindAndMove(block[index]);
      if (pos == 0)
        rleSize++;
      else
      {
        while (rleSize != 0)
        {
          rleSize--;
          mtfs[mtfArraySize++] = (rleSize & 1);
          symbolCounts[rleSize & 1]++;
          rleSize >>= 1;
        }
        if (pos >= 0xFE)
        {
          mtfs[mtfArraySize++] = 0xFF;
          mtfs[mtfArraySize++] = pos - 0xFE;
        }
        else
          mtfs[mtfArraySize++] = pos + 1;
        symbolCounts[pos + 1]++;
      }
    }
    while (++i < blockSize);

    while (rleSize != 0)
    {
      rleSize--;
      mtfs[mtfArraySize++] = (rleSize & 1);
      symbolCounts[rleSize & 1]++;
      rleSize >>= 1;
    }

    if (alphaSize < 256)
      mtfs[mtfArraySize++] = (Byte)(alphaSize - 1);
    else
    {
      mtfs[mtfArraySize++] = 0xFF;
      mtfs[mtfArraySize++] = (Byte)(alphaSize - 256);
    }
    symbolCounts[alphaSize - 1]++;
  }

  UInt32 numSymbols = 0;
  {
    for (int i = 0; i < kMaxAlphaSize; i++)
      numSymbols += symbolCounts[i];
  }

  int bestNumTables = kNumTablesMin;
  UInt32 bestPrice = 0xFFFFFFFF;
  UInt32 startPos = m_OutStreamCurrent->GetPos();
  UInt32 startCurByte = m_OutStreamCurrent->GetCurByte();
  for (int nt = kNumTablesMin; nt <= kNumTablesMax + 1; nt++)
  {
    int numTables;

    if(m_OptimizeNumTables)
    {
      m_OutStreamCurrent->SetPos(startPos);
      m_OutStreamCurrent->SetCurState((startPos & 7), startCurByte);
      if (nt <= kNumTablesMax)
        numTables = nt;
      else
        numTables = bestNumTables;
    }
    else
    {
      if (numSymbols < 200)  numTables = 2; 
      else if (numSymbols < 600) numTables = 3; 
      else if (numSymbols < 1200) numTables = 4; 
      else if (numSymbols < 2400) numTables = 5; 
      else numTables = 6;
    }

    WriteBits2(numTables, kNumTablesBits);
    
    UInt32 numSelectors = (numSymbols + kGroupSize - 1) / kGroupSize;
    WriteBits2(numSelectors, kNumSelectorsBits);
    
    {
      UInt32 remFreq = numSymbols;
      int gs = 0;
      int t = numTables;
      do
      {
        UInt32 tFreq = remFreq / t;
        int ge = gs;
        UInt32 aFreq = 0;
        while (aFreq < tFreq) //  && ge < alphaSize) 
          aFreq += symbolCounts[ge++];
        
        if (ge - 1 > gs && t != numTables && t != 1 && (((numTables - t) & 1) == 1)) 
          aFreq -= symbolCounts[--ge];
        
        NCompression::NHuffman::CEncoder &huffEncoder = m_HuffEncoders[t - 1];
        int i = 0;
        do
          huffEncoder.m_Items[i].Len = (i >= gs && i < ge) ? 0 : 1;
        while (++i < alphaSize);
        gs = ge;
        remFreq -= aFreq;
      }
      while(--t != 0);
    }
    
    
    for (int pass = 0; pass < kNumHuffPasses; pass++)
    {
      {
        int t = 0;
        do
          m_HuffEncoders[t].StartNewBlock();
        while(++t < numTables);
      }
      
      {
        UInt32 mtfPos = 0;
        UInt32 g = 0;
        do 
        {
          UInt32 symbols[kGroupSize];
          int i = 0;
          do
          {
            UInt32 symbol = mtfs[mtfPos++];
            if (symbol >= 0xFF)
              symbol += mtfs[mtfPos++];
            symbols[i] = symbol;
          }
          while (++i < kGroupSize && mtfPos < mtfArraySize);
          
          UInt32 bestPrice = 0xFFFFFFFF;
          int t = 0;
          do
          {
            NCompression::NHuffman::CItem *items = m_HuffEncoders[t].m_Items;
            UInt32 price = 0;
            int j = 0;
            do
              price += items[symbols[j]].Len;
            while (++j < i);
            if (price < bestPrice)
            {
              m_Selectors[g] = (Byte)t;
              bestPrice = price;
            }
          }
          while(++t < numTables);
          NCompression::NHuffman::CEncoder &huffEncoder = m_HuffEncoders[m_Selectors[g++]];
          int j = 0;
          do
            huffEncoder.AddSymbol(symbols[j]);
          while (++j < i);
        }
        while (mtfPos < mtfArraySize);
      }
      
      int t = 0;
      do
      {
        NCompression::NHuffman::CEncoder &huffEncoder = m_HuffEncoders[t];
        int i = 0;
        do
          if (huffEncoder.m_Items[i].Freq == 0)
            huffEncoder.m_Items[i].Freq = 1;
        while(++i < alphaSize);
        Byte levels[kMaxAlphaSize];
        huffEncoder.BuildTree(levels);
      }
      while(++t < numTables);
    }
    
    {
      Byte mtfSel[kNumTablesMax];
      {
        int t = 0;
        do
          mtfSel[t] = (Byte)t;
        while(++t < numTables);
      }
      
      UInt32 i = 0;
      do
      {
        Byte sel = m_Selectors[i];
        int pos;
        for (pos = 0; mtfSel[pos] != sel; pos++)
          WriteBit2(true);
        WriteBit2(false);
        for (; pos > 0; pos--)
          mtfSel[pos] = mtfSel[pos - 1];
        mtfSel[0] = sel;
      }
      while(++i < numSelectors);
    }
    
    {
      int t = 0;
      do
      {
        NCompression::NHuffman::CItem *items = m_HuffEncoders[t].m_Items;
        UInt32 len = items[0].Len;
        WriteBits2(len, kNumLevelsBits);
        int i = 0;
        do
        {
          UInt32 level = items[i].Len;
          while (len != level)
          {
            WriteBit2(true);
            if (len < level)
            {
              WriteBit2(false);
              len++;
            }
            else 
            {
              WriteBit2(true);
              len--;
            }
          }
          WriteBit2(false);
        }
        while (++i < alphaSize);
      }
      while(++t < numTables);
    }
    
    {
      UInt32 groupSize = 0;
      UInt32 groupIndex = 0;
      NCompression::NHuffman::CEncoder *huffEncoder = 0;
      UInt32 mtfPos = 0;
      do
      {
        UInt32 symbol = mtfs[mtfPos++];
        if (symbol >= 0xFF)
          symbol += mtfs[mtfPos++];
        if (groupSize == 0) 
        {
          groupSize = kGroupSize;
          huffEncoder = &m_HuffEncoders[m_Selectors[groupIndex++]];
        }
        groupSize--;                                    \
        huffEncoder->CodeOneValue(m_OutStreamCurrent, symbol);
      }
      while (mtfPos < mtfArraySize);
    }

    if (!m_OptimizeNumTables)
      break;
    UInt32 price = m_OutStreamCurrent->GetPos() - startPos;
    if (price <= bestPrice)
    {
      if (nt == kNumTablesMax)
        break;
      bestPrice = price;
      bestNumTables = nt;
    }
  }
}

// blockSize > 0
UInt32 CEncoder::EncodeBlockWithHeaders(Byte *block, UInt32 blockSize)
{
  WriteByte2(kBlockSig0);
  WriteByte2(kBlockSig1);
  WriteByte2(kBlockSig2);
  WriteByte2(kBlockSig3);
  WriteByte2(kBlockSig4);
  WriteByte2(kBlockSig5);

  CBZip2CRC crc;
  int numReps = 0;
  Byte prevByte = block[0];
  UInt32 i = 0;
  do 
  {
    Byte b = block[i];
    if (numReps == kRleModeRepSize)
    {
      for (; b > 0; b--)
        crc.UpdateByte(prevByte);
      numReps = 0;
      continue;
    }
    if (prevByte == b)
      numReps++;
    else
    {
      numReps = 1;
      prevByte = b;
    }
    crc.UpdateByte(b);
  }
  while (++i < blockSize);
  UInt32 crcRes = crc.GetDigest();
  WriteCRC2(crcRes);
  EncodeBlock(block, blockSize);
  return crcRes;
}

void CEncoder::EncodeBlock2(CBZip2CombinedCRC &combinedCRC, 
    Byte *block, UInt32 blockSize, UInt32 numPasses)
{
  bool needCompare = false;
  CBZip2CombinedCRC specCombinedCRC = combinedCRC;

  UInt32 startBytePos = m_OutStreamCurrent->GetBytePos();
  UInt32 startPos = m_OutStreamCurrent->GetPos();
  UInt32 startCurByte = m_OutStreamCurrent->GetCurByte();
  UInt32 endCurByte;
  UInt32 endPos;
  if (numPasses > 1 && blockSize >= (1 << 10))
  {
    UInt32 blockSize0 = blockSize / 2;
    for (;(block[blockSize0] == block[blockSize0 - 1] ||
          block[blockSize0 - 1] == block[blockSize0 - 2]) && 
          blockSize0 < blockSize; blockSize0++);
    if (blockSize0 < blockSize)
    {
      EncodeBlock2(specCombinedCRC, block, blockSize0, numPasses - 1);
      EncodeBlock2(specCombinedCRC, block + blockSize0, blockSize - blockSize0, 
          numPasses - 1);
      endPos = m_OutStreamCurrent->GetPos();
      endCurByte = m_OutStreamCurrent->GetCurByte();
      if ((endPos & 7) > 0)
        WriteBits2(0, 8 - (endPos & 7));
      m_OutStreamCurrent->SetCurState((startPos & 7), startCurByte);
      needCompare = true;
    }
  }

  UInt32 startBytePos2 = m_OutStreamCurrent->GetBytePos();
  UInt32 startPos2 = m_OutStreamCurrent->GetPos();
  UInt32 crcVal = EncodeBlockWithHeaders(block, blockSize);
  UInt32 endPos2 = m_OutStreamCurrent->GetPos();

  combinedCRC.Update(crcVal);

  if (needCompare)
  {
    UInt32 size2 = endPos2 - startPos2;
    if (size2 < endPos - startPos)
    { 
      UInt32 numBytes = m_OutStreamCurrent->GetBytePos() - startBytePos2;
      Byte *buffer = m_OutStreamCurrent->GetStream();
      for (UInt32 i = 0; i < numBytes; i++)
        buffer[startBytePos + i] = buffer[startBytePos2 + i];
      m_OutStreamCurrent->SetPos(startPos + endPos2 - startPos2);
    }
    else
    {
      m_OutStreamCurrent->SetPos(endPos);
      m_OutStreamCurrent->SetCurState((endPos & 7), endCurByte);
      combinedCRC = specCombinedCRC;
    }
  }
}

void CEncoder::EncodeBlock3(CBZip2CombinedCRC &combinedCRC, UInt32 blockSize)
{
  CMsbfEncoderTemp outStreamTemp;
  outStreamTemp.SetStream(m_TempArray);
  outStreamTemp.Init();
  m_OutStreamCurrent = &outStreamTemp;

  EncodeBlock2(combinedCRC, m_Block, blockSize, m_NumPasses);

  UInt32 size = outStreamTemp.GetPos();
  UInt32 bytesSize = (size / 8);
  for (UInt32 i = 0; i < bytesSize; i++)
    m_OutStream.WriteBits(m_TempArray[i], 8);
  WriteBits(outStreamTemp.GetCurByte(), (size & 7));
}

HRESULT CEncoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!m_BlockSorter.Create(kBlockSizeMax))
    return E_OUTOFMEMORY;
  
  if (m_Block == 0)
  {
    m_Block = (Byte *)BigAlloc(kBlockSizeMax * 5 + kBlockSizeMax / 10 + (20 << 10));
    if (m_Block == 0)
      return E_OUTOFMEMORY;
    m_MtfArray = m_Block + kBlockSizeMax;
    m_TempArray = m_MtfArray + kBlockSizeMax * 2 + 2;
  }
  if (!m_InStream.Create(kBufferSize))
    return E_OUTOFMEMORY;
  if (!m_OutStream.Create(kBufferSize))
    return E_OUTOFMEMORY;

  if (m_NeedHuffmanCreate)
  {
    for (int i = 0; i < kNumTablesMax; i++)
      if (!m_HuffEncoders[i].Create(kMaxAlphaSize, 0, 0, kMaxHuffmanLen))
        return E_OUTOFMEMORY;
    m_NeedHuffmanCreate = false;
  }

  m_InStream.SetStream(inStream);
  m_InStream.Init();

  m_OutStream.SetStream(outStream);
  m_OutStream.Init();

  CFlusher flusher(this);

  CBZip2CombinedCRC combinedCRC;

  WriteByte(kArSig0);
  WriteByte(kArSig1);
  WriteByte(kArSig2);
  WriteByte((Byte)(kArSig3 + m_BlockSizeMult));

  while (true)
  {
    UInt32 blockSize = ReadRleBlock(m_Block);
    if (blockSize == 0)
      break;
    EncodeBlock3(combinedCRC, blockSize);
    if (progress)
    {
      UInt64 packSize = m_InStream.GetProcessedSize();
      UInt64 unpackSize = m_OutStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &unpackSize));
    }
  }
  WriteByte(kFinSig0);
  WriteByte(kFinSig1);
  WriteByte(kFinSig2);
  WriteByte(kFinSig3);
  WriteByte(kFinSig4);
  WriteByte(kFinSig5);

  WriteCRC(combinedCRC.GetDigest());
  return S_OK;
}

STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(...) { return S_FALSE; }
}

HRESULT CEncoder::SetCoderProperties(const PROPID *propIDs, 
    const PROPVARIANT *properties, UInt32 numProperties)
{
  for(UInt32 i = 0; i < numProperties; i++)
  {
    const PROPVARIANT &property = properties[i]; 
    switch(propIDs[i])
    {
      case NCoderPropID::kNumPasses:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 numPasses = property.ulVal;
        if(numPasses == 0 || numPasses > 10)
          return E_INVALIDARG;
        m_NumPasses = numPasses;
        m_OptimizeNumTables = (m_NumPasses > 1);
        break;
      }
      case NCoderPropID::kDictionarySize:
      {
        if (property.vt != VT_UI4)
          return E_INVALIDARG;
        UInt32 dictionary = property.ulVal / kBlockSizeStep;
        if (dictionary < kBlockSizeMultMin)
          dictionary = kBlockSizeMultMin;
        else if (dictionary > kBlockSizeMultMax)
          dictionary = kBlockSizeMultMax;
        m_BlockSizeMult = dictionary;
        break;
      }
      default:
        return E_INVALIDARG;
    }
  }
  return S_OK;
}

}}
