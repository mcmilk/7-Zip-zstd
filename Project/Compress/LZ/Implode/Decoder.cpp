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


CCoder::CCoder():
  m_LiteralDecoder(kLiteralTableSize),
  m_LengthDecoder(kLengthTableSize),
  m_DistanceDecoder(kDistanceTableSize)
{
  m_OutWindowStream.Create(kHistorySize/*, kMatchMaxLenMax*/);
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

void CCoder::ReadLevelItems(NImplode::NHuffman::CDecoder &decoder, 
    BYTE *levels, int numLevelItems)
{
  int numCodedStructures = m_InBitStream.ReadBits(kNumBitsInByte) + 
      kLevelStructuresNumberAdditionalValue;
  int currentIndex = 0;
  for(int i = 0; i < numCodedStructures; i++)
  {
    int level = m_InBitStream.ReadBits(kNumLevelStructureLevelBits) + 
      kLevelStructureLevelAdditionalValue;
    int rep = m_InBitStream.ReadBits(kNumLevelStructureRepNumberBits) + 
      kLevelStructureRepNumberAdditionalValue;
    if (currentIndex + rep > numLevelItems)
      throw CException(CException::kData);
    for(int j = 0; j < rep; j++)
      levels[currentIndex++] = level;
  }
  if (currentIndex != numLevelItems)
    throw CException(CException::kData);
  try
  {
    decoder.SetCodeLengths(levels);
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
    BYTE literalLevels[kLiteralTableSize];
    ReadLevelItems(m_LiteralDecoder, literalLevels, kLiteralTableSize);
  }

  BYTE lengthLevels[kLengthTableSize];
  ReadLevelItems(m_LengthDecoder, lengthLevels, kLengthTableSize);

  BYTE distanceLevels[kDistanceTableSize];
  ReadLevelItems(m_DistanceDecoder, distanceLevels, kDistanceTableSize);

}

class CCoderReleaser
{
  CCoder *m_Coder;
public:
  CCoderReleaser(CCoder *coder): m_Coder(coder) {}
  ~CCoderReleaser() { m_Coder->ReleaseStreams(); }
};

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UINT64 pos = 0, unPackSize = *outSize;

  m_OutWindowStream.Init(outStream, false);
  m_InBitStream.Init(inStream);
  CCoderReleaser coderReleaser(this);

  ReadTables();
  
  while(pos < unPackSize)
  {
    if (progress != NULL && pos % (1 << 16) == 0)
    {
      UINT64 packSize = m_InBitStream.GetProcessedSize();
      RINOK(progress->SetRatioInfo(&packSize, &pos));
    }
    if(m_InBitStream.ReadBits(1) == kMatchId) // match
    {
      UINT32 lowDistBits = m_InBitStream.ReadBits(m_NumDistanceLowDirectBits);
      UINT32 distance = (m_DistanceDecoder.DecodeSymbol(&m_InBitStream) << 
          m_NumDistanceLowDirectBits) + lowDistBits;

      UINT32 lengthSymbol = m_LengthDecoder.DecodeSymbol(&m_InBitStream);
      UINT32 length = lengthSymbol + m_MinMatchLength;
      if (lengthSymbol == kLengthTableSize - 1) // special symbol  = 63
        length += m_InBitStream.ReadBits(kNumAdditionalLengthBits);
      while(distance >= pos && length > 0)
      {
        m_OutWindowStream.PutOneByte(0);
        pos++;
        length--;
      }
      if (length > 0)
        m_OutWindowStream.CopyBackBlock(distance, length);
      pos += length;
    }
    else
    {
      BYTE b = BYTE(m_LiteralsOn ? m_LiteralDecoder.DecodeSymbol(&m_InBitStream) : 
          m_InBitStream.ReadBits(kNumBitsInByte));
      m_OutWindowStream.PutOneByte(b);
      pos++;
    }
  }
  if (pos > unPackSize)
    throw CException(CException::kData);
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStream, outStream, inSize, outSize, progress);
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

STDMETHODIMP CCoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  BYTE flag;
  UINT32 processedSize;
  RINOK(inStream->Read(&flag, sizeof(flag), &processedSize));
  if (processedSize != sizeof(flag))
    return E_INVALIDARG;
  m_BigDictionaryOn = ((flag & 2) != 0);
  m_NumDistanceLowDirectBits = m_BigDictionaryOn ? 
      kNumDistanceLowDirectBitsForBigDict:
      kNumDistanceLowDirectBitsForSmallDict;
  m_LiteralsOn = ((flag & 4) != 0);
  m_MinMatchLength = m_LiteralsOn ? 
      kMatchMinLenWhenLiteralsOn : 
      kMatchMinLenWhenLiteralsOff;
  return S_OK;
}

}}
