// DeflateDecoder.cpp

#include "StdAfx.h"

#include "DeflateDecoder.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

CCoder::CCoder(bool deflate64Mode):  _deflate64Mode(deflate64Mode) {}

void CCoder::DeCodeLevelTable(Byte *newLevels, int numLevels)
{
  int i = 0;
  while (i < numLevels)
  {
    UInt32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number < kTableDirectLevels)
      newLevels[i++] = Byte(number);
    else
    {
      if (number == kTableLevelRepNumber)
      {
        int t = m_InBitStream.ReadBits(2) + 3;
        for (int reps = t; reps > 0 && i < numLevels ; reps--, i++)
          newLevels[i] = newLevels[i - 1];
      }
      else
      {
        int num;
        if (number == kTableLevel0Number)
          num = m_InBitStream.ReadBits(3) + 3;
        else
          num = m_InBitStream.ReadBits(7) + 11;
        for (;num > 0 && i < numLevels; num--)
          newLevels[i++] = 0;
      }
    }
  }
}

void CCoder::ReadTables(void)
{
  if(m_FinalBlock) // test it
    throw CException(CException::kData);

  m_FinalBlock = (m_InBitStream.ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);

  int blockType = m_InBitStream.ReadBits(kBlockTypeFieldSize);

  switch(blockType)
  {
    case NBlockType::kStored:
      {
        m_StoredMode = true;
        UInt32 currentBitPosition = m_InBitStream.GetBitPosition();
        UInt32 numBitsForAlign = currentBitPosition > 0 ? (8 - currentBitPosition): 0;
        if (numBitsForAlign > 0)
          m_InBitStream.ReadBits(numBitsForAlign);
        m_StoredBlockSize = m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize);
        UInt16 onesComplementReverse = ~(UInt16)(m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize));
        if (m_StoredBlockSize != onesComplementReverse)
          throw CException(CException::kData);
        break;
      }
    case NBlockType::kFixedHuffman:
    case NBlockType::kDynamicHuffman:
      {
        m_StoredMode = false;
        Byte litLenLevels[kStaticMainTableSize];
        Byte distLevels[kStaticDistTableSize];
        if (blockType == NBlockType::kFixedHuffman)
        {
          int i;

          // Leteral / length levels
          for (i = 0; i < 144; i++)
            litLenLevels[i] = 8;
          for (; i < 256; i++)
            litLenLevels[i] = 9;
          for (; i < 280; i++)
            litLenLevels[i] = 7;
          for (; i < 288; i++)          /* make a complete, but wrong code set */
            litLenLevels[i] = 8;
        
          // Distance levels
          for (i = 0; i < kStaticDistTableSize; i++)  // test it: infozip only use kDistTableSize       
            distLevels[i] = 5;
        }
        else // in case when (blockType == kDeflateBlockTypeFixedHuffman)
        {
          int numLitLenLevels = m_InBitStream.ReadBits(kDeflateNumberOfLengthCodesFieldSize) + 
            kDeflateNumberOfLitLenCodesMin;
          int numDistLevels = m_InBitStream.ReadBits(kDeflateNumberOfDistanceCodesFieldSize) + 
            kDeflateNumberOfDistanceCodesMin;
          int numLevelCodes = m_InBitStream.ReadBits(kDeflateNumberOfLevelCodesFieldSize) + 
            kDeflateNumberOfLevelCodesMin;
          
          int numLevels = _deflate64Mode ? kHeapTablesSizesSum64 :
            kHeapTablesSizesSum32;
          
          Byte levelLevels[kLevelTableSize];
          int i;
          for (i = 0; i < kLevelTableSize; i++)
          {
            int position = kCodeLengthAlphabetOrder[i]; 
            if(i < numLevelCodes)
              levelLevels[position] = Byte(m_InBitStream.ReadBits(kDeflateLevelCodeFieldSize));
            else
              levelLevels[position] = 0;
          }
          
          try
          {
            m_LevelDecoder.SetCodeLengths(levelLevels);
          }
          catch(...)
          {
            throw CException(CException::kData);
          }
          
          Byte tmpLevels[kStaticMaxTableSize];
          DeCodeLevelTable(tmpLevels, numLitLenLevels + numDistLevels);
          
          memmove(litLenLevels, tmpLevels, numLitLenLevels);
          memset(litLenLevels + numLitLenLevels, 0, 
            kStaticMainTableSize - numLitLenLevels);
          
          memmove(distLevels, tmpLevels + numLitLenLevels, numDistLevels);
          memset(distLevels + numDistLevels, 0, kStaticDistTableSize - numDistLevels);
        }
        try
        {
          m_MainDecoder.SetCodeLengths(litLenLevels);
          m_DistDecoder.SetCodeLengths(distLevels);
        }
        catch(...)
        {
          throw CException(CException::kData);
        }
        break;
      }
    default:
      throw CException(CException::kData);
  }
}

HRESULT CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64:  kHistorySize32))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 17))
    return E_OUTOFMEMORY;
  UInt64 pos = 0;
  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InBitStream.SetStream(inStream);
  m_InBitStream.Init();
  CCoderReleaser coderReleaser(this);

  m_FinalBlock = false;

  while(!m_FinalBlock)
  {
    if (progress != NULL)
    {
      UInt64 packSize = m_InBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos));
    }
    ReadTables();
    if(m_StoredMode)
    {
      for (UInt32 i = 0; i < m_StoredBlockSize; i++)
        m_OutWindowStream.PutByte(Byte(m_InBitStream.ReadBits(8)));
      pos += m_StoredBlockSize;
      continue;
    }
    while(true)
    {
      if (m_InBitStream.NumExtraBytes > 4)
        throw CException(CException::kData);

      UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number < 256)
      {
        if (outSize != NULL)
          if (pos >= *outSize)
            throw CException(CException::kData);
        m_OutWindowStream.PutByte(Byte(number));
        pos++;
        continue;
      }
      else if (number >= kMatchNumber)
      {
        if (outSize != NULL)
          if (pos >= *outSize)
            throw CException(CException::kData);
        number -= kMatchNumber;

        UInt32 length;
        if (_deflate64Mode)
        {
          length = UInt32(kLenStart64[number]) + kMatchMinLen;
          UInt32 numBits = kLenDirectBits64[number];
          if (numBits > 0)
            length += m_InBitStream.ReadBits(numBits);
        }
        else
        {
          length = UInt32(kLenStart32[number]) + kMatchMinLen;
          UInt32 numBits = kLenDirectBits32[number];
          if (numBits > 0)
            length += m_InBitStream.ReadBits(numBits);
        }

        
        number = m_DistDecoder.DecodeSymbol(&m_InBitStream);
        UInt32 distance = kDistStart[number] + m_InBitStream.ReadBits(kDistDirectBits[number]);
        if (distance >= pos)
          throw "data error";
         m_OutWindowStream.CopyBlock(distance, length);
        pos += length;
      }
      else if (number == kReadTableNumber)
        break;
      else
        throw CException(CException::kData);
    }
  }
  coderReleaser.NeedFlush = false;
  return m_OutWindowStream.Flush();
}

HRESULT CCoder::BaseCode(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLZOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

HRESULT CCoder::BaseGetInStreamProcessedSize(UInt64 *value)
{
  if (value == NULL)
    return E_INVALIDARG;
  *value = m_InBitStream.GetProcessedSize();
  return S_OK;
}

STDMETHODIMP CCOMCoder::GetInStreamProcessedSize(UInt64 *value)
{
  return BaseGetInStreamProcessedSize(value);
}

HRESULT CCOMCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  return BaseCode(inStream, outStream, inSize, outSize, progress);
}

STDMETHODIMP CCOMCoder64::GetInStreamProcessedSize(UInt64 *value)
{
  return BaseGetInStreamProcessedSize(value);
}

HRESULT CCOMCoder64::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  return BaseCode(inStream, outStream, inSize, outSize, progress);
}


}}}
