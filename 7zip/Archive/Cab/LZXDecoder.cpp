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
  m_i86TranslationOutStreamSpec = new Ci86TranslationOutStream;
  m_i86TranslationOutStream = m_i86TranslationOutStreamSpec;
}

void CDecoder::ReleaseStreams()
{
  // m_OutWindowStream.ReleaseStream();
  // m_InBitStream.ReleaseStream();
  m_i86TranslationOutStreamSpec->ReleaseStream();
}

STDMETHODIMP CDecoder::Flush()
{
  RINOK(m_OutWindowStream.Flush());
  return m_i86TranslationOutStreamSpec->Flush();
}

void CDecoder::ReadTable(BYTE *lastLevels, BYTE *newLevels, UINT32 numSymbols)
{
  BYTE levelLevels[kLevelTableSize];
  UINT32 i;
  for (i = 0; i < kLevelTableSize; i++)
    levelLevels[i] = BYTE(m_InBitStream.ReadBits(kNumBitsForPreTreeLevel));
  m_LevelDecoder.SetCodeLengths(levelLevels);
  for (i = 0; i < numSymbols;)
  {
    UINT32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number <= kNumHuffmanBits)
      newLevels[i++] = BYTE((17 + lastLevels[i] - number) % (kNumHuffmanBits + 1));
    else if (number == kLevelSymbolZeros || number == kLevelSymbolZerosBig)
    {
      int num;
      if (number == kLevelSymbolZeros)
        num = kLevelSymbolZerosStartValue + 
            m_InBitStream.ReadBits(kLevelSymbolZerosNumBits);
      else
        num = kLevelSymbolZerosBigStartValue + 
            m_InBitStream.ReadBits(kLevelSymbolZerosBigNumBits);
      for (;num > 0 && i < numSymbols; num--, i++)
        newLevels[i] = 0;
    }
    else if (number == kLevelSymbolSame)
    {
      int num = kLevelSymbolSameStartValue + m_InBitStream.ReadBits(kLevelSymbolSameNumBits);
      UINT32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
      if (number > kNumHuffmanBits)
        throw "bad data";
      BYTE symbol = BYTE((17 + lastLevels[i] - number) % (kNumHuffmanBits + 1));
      for (; num > 0 && i < numSymbols; num--, i++)
        newLevels[i] = symbol;
    }
    else
        throw "bad data";
  }

  memmove(lastLevels, newLevels, numSymbols);
}

void CDecoder::ReadTables(void)
{
  int blockType = m_InBitStream.ReadBits(NBlockType::kNumBits);

  if (blockType != NBlockType::kVerbatim && blockType != NBlockType::kAligned && 
      blockType != NBlockType::kUncompressed)
    throw "bad data";

  m_UnCompressedBlockSize = m_InBitStream.ReadBitsBig(kUncompressedBlockSizeNumBits);

  if (blockType == NBlockType::kUncompressed)
  {
    m_UncompressedBlock = true;
    UINT32 bitPos = m_InBitStream.GetBitPosition() % 16;
    m_InBitStream.ReadBits(16 - bitPos);
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

  m_AlignIsUsed = (blockType == NBlockType::kAligned);

  BYTE newLevels[kMaxTableSize];

  if (m_AlignIsUsed)
  {
    for(int i = 0; i < kAlignTableSize; i++)
      newLevels[i] = m_InBitStream.ReadBits(kNumBitsForAlignLevel);
    m_AlignDecoder.SetCodeLengths(newLevels);
  }

  ReadTable(m_LastByteLevels, newLevels, 256);
  ReadTable(m_LastPosLenLevels, newLevels + 256, m_NumPosLenSlots);
  for (int i = m_NumPosLenSlots; i < kNumPosSlotLenSlotSymbols; i++)
    newLevels[256 + i] = 0;
  m_MainDecoder.SetCodeLengths(newLevels);

  ReadTable(m_LastLenLevels, newLevels, kNumLenSymbols);
  m_LenDecoder.SetCodeLengths(newLevels);
  
}

class CDecoderFlusher
{
  CDecoder *m_Decoder;
public:
  CDecoderFlusher(CDecoder *decoder): m_Decoder(decoder) {}
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


STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UINT64 size = *outSize;


  m_OutWindowStream.Init(m_i86TranslationOutStream);
  m_InBitStream.InitStream(inStream, m_ReservedSize, m_NumInDataBlocks);

  CDecoderFlusher flusher(this);

  UINT32 uncompressedCFDataBlockSize;
  bool dataAreCorrect;
  RINOK(m_InBitStream.ReadBlock(uncompressedCFDataBlockSize, dataAreCorrect));
  if (!dataAreCorrect)
  {
    throw "Data Error";
  }
  UINT32 uncompressedCFDataCurrentValue = 0;
  m_InBitStream.Init();

  ClearPrevLeveles();

  if (m_InBitStream.ReadBits(1) == 0)
    m_i86TranslationOutStreamSpec->Init(outStream, false, 0);
  else
  {
    UINT32 i86TranslationSize = m_InBitStream.ReadBits(16) << 16;
    i86TranslationSize |= m_InBitStream.ReadBits(16);
    m_i86TranslationOutStreamSpec->Init(outStream, true , i86TranslationSize);
  }
  
  for(int i = 0 ; i < kNumRepDistances; i++)
    m_RepDistances[i] = 0;

  UINT64 nowPos64 = 0;
  while(nowPos64 < size)
  {
    if (uncompressedCFDataCurrentValue == uncompressedCFDataBlockSize)
    {
      bool dataAreCorrect;
      RINOK(m_InBitStream.ReadBlock(uncompressedCFDataBlockSize, dataAreCorrect));
      if (!dataAreCorrect)
      {
        throw "Data Error";
      }
      m_InBitStream.Init();
      uncompressedCFDataCurrentValue = 0;
    }
    ReadTables();
    UINT32 nowPos = 0;
    UINT32 next = (UINT32)MyMin((UINT64)m_UnCompressedBlockSize, size - nowPos64);
    if (m_UncompressedBlock)
    {
      while(nowPos < next)
      {
        m_OutWindowStream.PutOneByte(m_InBitStream.DirectReadByte());
        nowPos++;
        uncompressedCFDataCurrentValue++;
        if (uncompressedCFDataCurrentValue == uncompressedCFDataBlockSize)
        {
          bool dataAreCorrect;
          RINOK(m_InBitStream.ReadBlock(uncompressedCFDataBlockSize, dataAreCorrect));
          if (!dataAreCorrect)
          {
            throw "Data Error";
          }
          // m_InBitStream.Init();
          uncompressedCFDataCurrentValue = 0;
          continue;
        }
      }
      int bitPos = m_InBitStream.GetBitPosition() % 16;
      if (bitPos == 8)
        m_InBitStream.DirectReadByte();
      m_InBitStream.Normalize();
    }
    else for (;nowPos < next;)
    {
      if (uncompressedCFDataCurrentValue == uncompressedCFDataBlockSize)
      {
        bool dataAreCorrect;
        RINOK(m_InBitStream.ReadBlock(uncompressedCFDataBlockSize, dataAreCorrect));
        if (!dataAreCorrect)
        {
          throw "Data Error";
        }
        m_InBitStream.Init();
        uncompressedCFDataCurrentValue = 0;
      }
      UINT32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number < 256)
      {
        m_OutWindowStream.PutOneByte(BYTE(number));
        nowPos++;
        uncompressedCFDataCurrentValue++;
        // continue;
      }
      else if (number < 256 + m_NumPosLenSlots)
      {
        UINT32 posLenSlot =  number - 256;
        UINT32 posSlot = posLenSlot / kNumLenSlots;
        UINT32 lenSlot = posLenSlot % kNumLenSlots;
        UINT32 length = 2 + lenSlot;
        if (lenSlot == kNumLenSlots - 1)
          length += m_LenDecoder.DecodeSymbol(&m_InBitStream);
        
        if (posSlot < kNumRepDistances)
        {
          UINT32 distance = m_RepDistances[posSlot];
          m_OutWindowStream.CopyBackBlock(distance, length);
          if (posSlot != 0)
          {
            m_RepDistances[posSlot] = m_RepDistances[0];
            m_RepDistances[0] = distance;
          }
        }
        else
        {
          UINT32 pos = kDistStart[posSlot];
          UINT32 posDirectBits = kDistDirectBits[posSlot];
          if (m_AlignIsUsed && posDirectBits >= kNumAlignBits)
          {
            pos += (m_InBitStream.ReadBits(posDirectBits - kNumAlignBits) << kNumAlignBits);
            pos += m_AlignDecoder.DecodeSymbol(&m_InBitStream);
          }
          else
            pos += m_InBitStream.ReadBits(posDirectBits);
          UINT32 distance = pos - kNumRepDistances;
          if (distance >= nowPos64 + nowPos)
            throw 777123;
          m_OutWindowStream.CopyBackBlock(distance, length);
          m_RepDistances[2] = m_RepDistances[1];
          m_RepDistances[1] = m_RepDistances[0];
          m_RepDistances[0] = distance;
        }
        nowPos += length;
        uncompressedCFDataCurrentValue += length;
      }
      else
        throw 98112823;
    }
    if (progress != NULL)
    {
      UINT64 inSize = m_InBitStream.GetProcessedSize();
      UINT64 outSize = nowPos64 + nowPos;
      RINOK(progress->SetRatioInfo(&inSize, &outSize));
    }
    nowPos64 += nowPos;
  }
  return S_OK;
}

}}}
