// Archive/Cab/LZXDecoder.cpp

#include "StdAfx.h"

#include "LZXDecoder.h"

#include "LZXConst.h"
#include "Common/Defs.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {
namespace NLZX {

static const UINT32 kHistorySize = (1 << 21);

const kMainTableSize = 256 + kNumPosSlotLenSlotSymbols;

CDecoder::CDecoder():
  m_MainDecoder(kMainTableSize),
  m_LenDecoder(kNumLenSymbols),
  m_AlignDecoder(kAlignTableSize),
  m_LevelDecoder(kLevelTableSize)
{
  m_OutWindowStream.Create(kHistorySize);
  m_i86TranslationOutStreamSpec = new CComObjectNoLock<Ci86TranslationOutStream>;
  m_i86TranslationOutStream = m_i86TranslationOutStreamSpec;
}

void CDecoder::ReleaseStreams()
{
  m_OutWindowStream.ReleaseStream();
  m_InBitStream.ReleaseStream();
  m_i86TranslationOutStreamSpec->ReleaseStream();
}

STDMETHODIMP CDecoder::Flush()
{
  RETURN_IF_NOT_S_OK(m_OutWindowStream.Flush());
  return m_i86TranslationOutStreamSpec->Flush();
}

void CDecoder::ReadTable(BYTE *aLastLevels, BYTE *aNewLevels, UINT32 aNumSymbols)
{
  BYTE aLevelLevels[kLevelTableSize];
  for (int i = 0; i < kLevelTableSize; i++)
    aLevelLevels[i] = BYTE(m_InBitStream.ReadBits(kNumBitsForPreTreeLevel));
  m_LevelDecoder.SetCodeLengths(aLevelLevels);
  for (i = 0; i < aNumSymbols;)
  {
    UINT32 aNumber = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (aNumber <= kNumHuffmanBits)
      aNewLevels[i++] = BYTE((17 + aLastLevels[i] - aNumber) % (kNumHuffmanBits + 1));
    else if (aNumber == kLevelSymbolZeros || aNumber == kLevelSymbolZerosBig)
    {
      int aNum;
      if (aNumber == kLevelSymbolZeros)
        aNum = kLevelSymbolZerosStartValue + 
            m_InBitStream.ReadBits(kLevelSymbolZerosNumBits);
      else
        aNum = kLevelSymbolZerosBigStartValue + 
            m_InBitStream.ReadBits(kLevelSymbolZerosBigNumBits);
      for (;aNum > 0 && i < aNumSymbols; aNum--, i++)
        aNewLevels[i] = 0;
    }
    else if (aNumber == kLevelSymbolSame)
    {
      int aNum = kLevelSymbolSameStartValue + m_InBitStream.ReadBits(kLevelSymbolSameNumBits);
      UINT32 aNumber = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
      if (aNumber > kNumHuffmanBits)
        throw "bad data";
      BYTE aSymbol = BYTE((17 + aLastLevels[i] - aNumber) % (kNumHuffmanBits + 1));
      for (; aNum > 0 && i < aNumSymbols; aNum--, i++)
        aNewLevels[i] = aSymbol;
    }
    else
        throw "bad data";
  }

  memmove(aLastLevels, aNewLevels, aNumSymbols);
}

void CDecoder::ReadTables(void)
{
  int aBlockType = m_InBitStream.ReadBits(NBlockType::kNumBits);

  if (aBlockType != NBlockType::kVerbatim && aBlockType != NBlockType::kAligned && 
      aBlockType != NBlockType::kUncompressed)
    throw "bad data";

  m_UnCompressedBlockSize = m_InBitStream.ReadBitsBig(kUncompressedBlockSizeNumBits);

  if (aBlockType == NBlockType::kUncompressed)
  {
    m_UncompressedBlock = true;
    UINT32 aBitPos = m_InBitStream.GetBitPosition() % 16;
    m_InBitStream.ReadBits(16 - aBitPos);
    for (int i = 0; i < kNumRepDistances; i++)
    {
      m_RepDistances[i] = 0;
      for (int j = 0; j < 4; j++)
        m_RepDistances[i] |= (m_InBitStream.DirectReadByte()) << (8 * j);
      m_RepDistances[i]--;
    }
    return;
  }
  
  m_UncompressedBlock = false;

  m_AlignIsUsed = (aBlockType == NBlockType::kAligned);

  BYTE aNewLevels[kMaxTableSize];

  if (m_AlignIsUsed)
  {
    for(int i = 0; i < kAlignTableSize; i++)
      aNewLevels[i] = m_InBitStream.ReadBits(kNumBitsForAlignLevel);
    m_AlignDecoder.SetCodeLengths(aNewLevels);
  }

  ReadTable(m_LastByteLevels, aNewLevels, 256);
  ReadTable(m_LastPosLenLevels, aNewLevels + 256, m_NumPosLenSlots);
  for (int i = m_NumPosLenSlots; i < kNumPosSlotLenSlotSymbols; i++)
    aNewLevels[256 + i] = 0;
  m_MainDecoder.SetCodeLengths(aNewLevels);

  ReadTable(m_LastLenLevels, aNewLevels, kNumLenSymbols);
  m_LenDecoder.SetCodeLengths(aNewLevels);
  
}

class CDecoderFlusher
{
  CDecoder *m_Decoder;
public:
  CDecoderFlusher(CDecoder *aDecoder): m_Decoder(aDecoder) {}
  ~CDecoderFlusher()
  {
    m_Decoder->Flush();
    m_Decoder->ReleaseStreams();
  }
};


void CDecoder::ClearPrevLeveles()
{
  memset(m_LastByteLevels, 0, 256);
  memset(m_LastPosLenLevels, 0, kNumPosSlotLenSlotSymbols);
  memset(m_LastLenLevels, 0, kNumLenSymbols);
};


STDMETHODIMP CDecoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;
  UINT64 aSize = *anOutSize;


  m_OutWindowStream.Init(m_i86TranslationOutStream);
  m_InBitStream.InitStream(anInStream, m_ReservedSize, m_NumInDataBlocks);

  CDecoderFlusher aFlusher(this);

  UINT32 anUncompressedCFDataBlockSize;
  bool aDataAreCorrect;
  RETURN_IF_NOT_S_OK(m_InBitStream.ReadBlock(anUncompressedCFDataBlockSize, aDataAreCorrect));
  if (!aDataAreCorrect)
  {
    throw "Data Error";
  }
  UINT32 anUncompressedCFDataCurrentValue = 0;
  m_InBitStream.Init();

  ClearPrevLeveles();

  if (m_InBitStream.ReadBits(1) == 0)
    m_i86TranslationOutStreamSpec->Init(anOutStream, false, 0);
  else
  {
    UINT32 ani86TranslationSize = m_InBitStream.ReadBits(16) << 16;
    ani86TranslationSize |= m_InBitStream.ReadBits(16);
    m_i86TranslationOutStreamSpec->Init(anOutStream, true , ani86TranslationSize);
  }
  

  for(int i = 0 ; i < kNumRepDistances; i++)
    m_RepDistances[i] = 0;

  UINT64 aNowPos64 = 0;
  while(aNowPos64 < aSize)
  {
    if (anUncompressedCFDataCurrentValue == anUncompressedCFDataBlockSize)
    {
      bool aDataAreCorrect;
      RETURN_IF_NOT_S_OK(m_InBitStream.ReadBlock(anUncompressedCFDataBlockSize, aDataAreCorrect));
      if (!aDataAreCorrect)
      {
        throw "Data Error";
      }
      m_InBitStream.Init();
      anUncompressedCFDataCurrentValue = 0;
    }
    ReadTables();
    UINT32 aNowPos = 0;
    UINT32 aNext = (UINT32)MyMin((UINT64)m_UnCompressedBlockSize, aSize - aNowPos64);
    if (m_UncompressedBlock)
    {
      while(aNowPos < aNext)
      {
        m_OutWindowStream.PutOneByte(m_InBitStream.DirectReadByte());
        aNowPos++;
        anUncompressedCFDataCurrentValue++;
        if (anUncompressedCFDataCurrentValue == anUncompressedCFDataBlockSize)
        {
          bool aDataAreCorrect;
          RETURN_IF_NOT_S_OK(m_InBitStream.ReadBlock(anUncompressedCFDataBlockSize, aDataAreCorrect));
          if (!aDataAreCorrect)
          {
            throw "Data Error";
          }
          // m_InBitStream.Init();
          anUncompressedCFDataCurrentValue = 0;
          continue;
        }
      }
      int aBitPos = m_InBitStream.GetBitPosition() % 16;
      if (aBitPos == 8)
        m_InBitStream.DirectReadByte();
      m_InBitStream.Normalize();
    }
    else for (;aNowPos < aNext;)
    {
      if (anUncompressedCFDataCurrentValue == anUncompressedCFDataBlockSize)
      {
        bool aDataAreCorrect;
        RETURN_IF_NOT_S_OK(m_InBitStream.ReadBlock(anUncompressedCFDataBlockSize, aDataAreCorrect));
        if (!aDataAreCorrect)
        {
          throw "Data Error";
        }
        m_InBitStream.Init();
        anUncompressedCFDataCurrentValue = 0;
      }
      UINT32 aNumber = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (aNumber < 256)
      {
        m_OutWindowStream.PutOneByte(BYTE(aNumber));
        aNowPos++;
        anUncompressedCFDataCurrentValue++;
        // continue;
      }
      else if (aNumber < 256 + m_NumPosLenSlots)
      {
        UINT32 aPosLenSlot =  aNumber - 256;
        UINT32 aPosSlot = aPosLenSlot / kNumLenSlots;
        UINT32 aLenSlot = aPosLenSlot % kNumLenSlots;
        UINT32 aLength = 2 + aLenSlot;
        if (aLenSlot == kNumLenSlots - 1)
          aLength += m_LenDecoder.DecodeSymbol(&m_InBitStream);
        
        if (aPosSlot < kNumRepDistances)
        {
          UINT32 aDistance = m_RepDistances[aPosSlot];
          m_OutWindowStream.CopyBackBlock(aDistance, aLength);
          if (aPosSlot != 0)
          {
            m_RepDistances[aPosSlot] = m_RepDistances[0];
            m_RepDistances[0] = aDistance;
          }
        }
        else
        {
          UINT32 aPos = kDistStart[aPosSlot];
          UINT32 aPosDirectBits = kDistDirectBits[aPosSlot];
          if (m_AlignIsUsed && aPosDirectBits >= kNumAlignBits)
          {
            aPos += (m_InBitStream.ReadBits(aPosDirectBits - kNumAlignBits) << kNumAlignBits);
            aPos += m_AlignDecoder.DecodeSymbol(&m_InBitStream);
          }
          else
            aPos += m_InBitStream.ReadBits(aPosDirectBits);
          UINT32 aDistance = aPos - kNumRepDistances;
          if (aDistance >= aNowPos64 + aNowPos)
            throw 777123;
          m_OutWindowStream.CopyBackBlock(aDistance, aLength);
          m_RepDistances[2] = m_RepDistances[1];
          m_RepDistances[1] = m_RepDistances[0];
          m_RepDistances[0] = aDistance;
        }
        aNowPos += aLength;
        anUncompressedCFDataCurrentValue += aLength;
      }
      else
        throw 98112823;
    }
    if (aProgress != NULL)
    {
      UINT64 anInSize = m_InBitStream.GetProcessedSize();
      UINT64 anOutSize = aNowPos64 + aNowPos;
      HRESULT aResult = aProgress->SetRatioInfo(&anInSize, &anOutSize);
      if (aResult != S_OK)
        return aResult;
    }
    aNowPos64 += aNowPos;
  }
  return S_OK;
}

}}}
