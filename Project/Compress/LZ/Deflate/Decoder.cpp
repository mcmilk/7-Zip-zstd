// Deflate/Decoder.cpp

#include "StdAfx.h"

#include "Decoder.h"
#include "Const.h"

#include "Windows/Defs.h"

namespace NDeflate {
namespace NDecoder {

CCoder::CCoder():
  m_MainDecoder(kStaticMainTableSize),
  m_DistDecoder(kStaticDistTableSize),
  m_LevelDecoder(kLevelTableSize)
{}

void CCoder::DeCodeLevelTable(BYTE *aNewLevels, int aNumLevels)
{
  int i = 0;
  while (i < aNumLevels)
  {
    UINT32 aNumber = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (aNumber < kTableDirectLevels)
      aNewLevels[i++] = BYTE(aNumber);
    else
    {
      if (aNumber == kTableLevelRepNumber)
      {
        int t = m_InBitStream.ReadBits(2) + 3;
        for (int aReps = t; aReps > 0 && i < aNumLevels ; aReps--, i++)
          aNewLevels[i] = aNewLevels[i - 1];
      }
      else
      {
        int aNum;
        if (aNumber == kTableLevel0Number)
          aNum = m_InBitStream.ReadBits(3) + 3;
        else
          aNum = m_InBitStream.ReadBits(7) + 11;
        for (;aNum > 0 && i < aNumLevels; aNum--)
          aNewLevels[i++] = 0;
      }
    }
  }
}

void CCoder::ReadTables(void)
{
  if(m_FinalBlock) // test it
    throw CException(CException::kData);

  m_FinalBlock = (m_InBitStream.ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);

  int aBlockType = m_InBitStream.ReadBits(kBlockTypeFieldSize);

  switch(aBlockType)
  {
    case NBlockType::kStored:
      {
        m_StoredMode = true;
        UINT32 aCurrentBitPosition = m_InBitStream.GetBitPosition();
        UINT32 aNumBitsForAlign = aCurrentBitPosition > 0 ? (8 - aCurrentBitPosition): 0;
        if (aNumBitsForAlign > 0)
          m_InBitStream.ReadBits(aNumBitsForAlign);
        m_StoredBlockSize = m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize);
        WORD anOnesComplementReverse = ~WORD(m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize));
        if (m_StoredBlockSize != anOnesComplementReverse)
          throw CException(CException::kData);
        break;
      }
    case NBlockType::kFixedHuffman:
    case NBlockType::kDynamicHuffman:
      {
        m_StoredMode = false;
        BYTE aLitLenLevels[kStaticMainTableSize];
        BYTE aDistLevels[kStaticDistTableSize];
        if (aBlockType == NBlockType::kFixedHuffman)
        {
          int i;

          // Leteral / length levels
          for (i = 0; i < 144; i++)
            aLitLenLevels[i] = 8;
          for (; i < 256; i++)
            aLitLenLevels[i] = 9;
          for (; i < 280; i++)
            aLitLenLevels[i] = 7;
          for (; i < 288; i++)          /* make a complete, but wrong code set */
            aLitLenLevels[i] = 8;
        
          // Distance levels
          for (i = 0; i < kStaticDistTableSize; i++)  // test it: infozip only use kDistTableSize       
            aDistLevels[i] = 5;
        }
        else // in case when (aBlockType == kDeflateBlockTypeFixedHuffman)
        {
          int aNumLitLenLevels = m_InBitStream.ReadBits(kDeflateNumberOfLengthCodesFieldSize) + 
            kDeflateNumberOfLitLenCodesMin;
          int aNumDistLevels = m_InBitStream.ReadBits(kDeflateNumberOfDistanceCodesFieldSize) + 
            kDeflateNumberOfDistanceCodesMin;
          int aNumLevelCodes = m_InBitStream.ReadBits(kDeflateNumberOfLevelCodesFieldSize) + 
            kDeflateNumberOfLevelCodesMin;
          
          int aNumLevels;
          aNumLevels = kHeapTablesSizesSum;
          
          BYTE aLevelLevels[kLevelTableSize];
          int i;
          for (i = 0; i < kLevelTableSize; i++)
          {
            int aPosition = kCodeLengthAlphabetOrder[i]; 
            if(i < aNumLevelCodes)
              aLevelLevels[aPosition] = BYTE(m_InBitStream.ReadBits(kDeflateLevelCodeFieldSize));
            else
              aLevelLevels[aPosition] = 0;
          }
          
          try
          {
            m_LevelDecoder.SetCodeLengths(aLevelLevels);
          }
          catch(...)
          {
            throw CException(CException::kData);
          }
          
          BYTE aTmpLevels[kStaticMaxTableSize];
          DeCodeLevelTable(aTmpLevels, aNumLitLenLevels + aNumDistLevels);
          
          memmove(aLitLenLevels, aTmpLevels, aNumLitLenLevels);
          memset(aLitLenLevels + aNumLitLenLevels, 0, 
            kStaticMainTableSize - aNumLitLenLevels);
          
          memmove(aDistLevels, aTmpLevels + aNumLitLenLevels, aNumDistLevels);
          memset(aDistLevels + aNumDistLevels, 0, kStaticDistTableSize - aNumDistLevels);
        }
        try
        {
          m_MainDecoder.SetCodeLengths(aLitLenLevels);
          m_DistDecoder.SetCodeLengths(aDistLevels);
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

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  {
    try
    {
      m_OutWindowStream.Create(kHistorySize /*, kMatchMaxLen */);
    }
    catch(...)
    {
      return E_OUTOFMEMORY;
    }
  }
  UINT64 aPos = 0;
  m_OutWindowStream.Init(anOutStream, false);
  m_InBitStream.Init(anInStream);
  CCoderReleaser aCoderReleaser(this);

  m_FinalBlock = false;

  while(!m_FinalBlock)
  {
    if (aProgress != NULL)
    {
      UINT64 aPackSize = m_InBitStream.GetProcessedSize();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aPackSize, &aPos));
    }
    ReadTables();
    if(m_StoredMode)
    {
      for (UINT32 i = 0; i < m_StoredBlockSize; i++)
        m_OutWindowStream.PutOneByte(BYTE(m_InBitStream.ReadBits(8)));
      aPos += m_StoredBlockSize;
      continue;
    }
    while(true)
    {
      UINT32 aNumber = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (aNumber < 256)
      {
        if (anOutSize != NULL)
          if (aPos >= *anOutSize)
            throw CException(CException::kData);
        m_OutWindowStream.PutOneByte(BYTE(aNumber));
        aPos++;
        continue;
      }
      else if (aNumber >= kMatchNumber)
      {
        if (anOutSize != NULL)
          if (aPos >= *anOutSize)
            throw CException(CException::kData);
        aNumber -= kMatchNumber;
        UINT32 aLength = UINT32(kLenStart[aNumber]) + kMatchMinLen;
        UINT32 aNumBits; 
        if ((aNumBits = kLenDirectBits[aNumber]) > 0)
          aLength += m_InBitStream.ReadBits(aNumBits);
        
        aNumber = m_DistDecoder.DecodeSymbol(&m_InBitStream);
        UINT32 aDistance = kDistStart[aNumber] + m_InBitStream.ReadBits(kDistDirectBits[aNumber]);
        if (aDistance >= aPos)
          throw "data error";
         m_OutWindowStream.CopyBackBlock(aDistance, aLength);
        aPos += aLength;
      }
      else if (aNumber == kReadTableNumber)
        break;
      else
        throw CException(CException::kData);
    }
  }
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
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
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

STDMETHODIMP CCoder::GetInStreamProcessedSize(UINT64 *aValue)
{
  if (aValue == NULL)
    return E_INVALIDARG;
  *aValue = m_InBitStream.GetProcessedSize();
  return S_OK;
}

}}
