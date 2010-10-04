// LzxDecoder.cpp

#include "StdAfx.h"

#include "../../Common/Defs.h"

#include "LzxDecoder.h"

namespace NCompress {
namespace NLzx {

const int kLenIdNeedInit = -2;

CDecoder::CDecoder(bool wimMode):
  _keepHistory(false),
  _skipByte(false),
  _wimMode(wimMode)
{
  m_x86ConvertOutStreamSpec = new Cx86ConvertOutStream;
  m_x86ConvertOutStream = m_x86ConvertOutStreamSpec;
}

void CDecoder::ReleaseStreams()
{
  m_OutWindowStream.ReleaseStream();
  m_InBitStream.ReleaseStream();
  m_x86ConvertOutStreamSpec->ReleaseStream();
}

STDMETHODIMP CDecoder::Flush()
{
  RINOK(m_OutWindowStream.Flush());
  return m_x86ConvertOutStreamSpec->Flush();
}

UInt32 CDecoder::ReadBits(unsigned numBits) { return m_InBitStream.ReadBits(numBits); }

#define RIF(x) { if (!(x)) return false; }

bool CDecoder::ReadTable(Byte *lastLevels, Byte *newLevels, UInt32 numSymbols)
{
  Byte levelLevels[kLevelTableSize];
  UInt32 i;
  for (i = 0; i < kLevelTableSize; i++)
    levelLevels[i] = (Byte)ReadBits(kNumBitsForPreTreeLevel);
  RIF(m_LevelDecoder.SetCodeLengths(levelLevels));
  unsigned num = 0;
  Byte symbol = 0;
  for (i = 0; i < numSymbols;)
  {
    if (num != 0)
    {
      lastLevels[i] = newLevels[i] = symbol;
      i++;
      num--;
      continue;
    }
    UInt32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number == kLevelSymbolZeros)
    {
      num = kLevelSymbolZerosStartValue + (unsigned)ReadBits(kLevelSymbolZerosNumBits);
      symbol = 0;
    }
    else if (number == kLevelSymbolZerosBig)
    {
      num = kLevelSymbolZerosBigStartValue + (unsigned)ReadBits(kLevelSymbolZerosBigNumBits);
      symbol = 0;
    }
    else if (number == kLevelSymbolSame || number <= kNumHuffmanBits)
    {
      if (number <= kNumHuffmanBits)
        num = 1;
      else
      {
        num = kLevelSymbolSameStartValue + (unsigned)ReadBits(kLevelSymbolSameNumBits);
        number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
        if (number > kNumHuffmanBits)
          return false;
      }
      symbol = Byte((17 + lastLevels[i] - number) % (kNumHuffmanBits + 1));
    }
    else
      return false;
  }
  return true;
}

bool CDecoder::ReadTables(void)
{
  Byte newLevels[kMaxTableSize];
  {
    if (_skipByte)
      m_InBitStream.DirectReadByte();
    m_InBitStream.Normalize();

    unsigned blockType = (unsigned)ReadBits(kNumBlockTypeBits);
    if (blockType > kBlockTypeUncompressed)
      return false;
    if (_wimMode)
      if (ReadBits(1) == 1)
        m_UnCompressedBlockSize = (1 << 15);
      else
        m_UnCompressedBlockSize = ReadBits(16);
    else
      m_UnCompressedBlockSize = m_InBitStream.ReadBitsBig(kUncompressedBlockSizeNumBits);

    m_IsUncompressedBlock = (blockType == kBlockTypeUncompressed);

    _skipByte = (m_IsUncompressedBlock && ((m_UnCompressedBlockSize & 1) != 0));

    if (m_IsUncompressedBlock)
    {
      ReadBits(16 - m_InBitStream.GetBitPosition());
      if (!m_InBitStream.ReadUInt32(m_RepDistances[0]))
        return false;
      m_RepDistances[0]--;
      for (unsigned i = 1; i < kNumRepDistances; i++)
      {
        UInt32 rep = 0;
        for (unsigned j = 0; j < 4; j++)
          rep |= (UInt32)m_InBitStream.DirectReadByte() << (8 * j);
        m_RepDistances[i] = rep - 1;
      }
      return true;
    }
    m_AlignIsUsed = (blockType == kBlockTypeAligned);
    if (m_AlignIsUsed)
    {
      for (unsigned i = 0; i < kAlignTableSize; i++)
        newLevels[i] = (Byte)ReadBits(kNumBitsForAlignLevel);
      RIF(m_AlignDecoder.SetCodeLengths(newLevels));
    }
  }

  RIF(ReadTable(m_LastMainLevels, newLevels, 256));
  RIF(ReadTable(m_LastMainLevels + 256, newLevels + 256, m_NumPosLenSlots));
  for (UInt32 i = 256 + m_NumPosLenSlots; i < kMainTableSize; i++)
    newLevels[i] = 0;
  RIF(m_MainDecoder.SetCodeLengths(newLevels));

  RIF(ReadTable(m_LastLenLevels, newLevels, kNumLenSymbols));
  return m_LenDecoder.SetCodeLengths(newLevels);
}

class CDecoderFlusher
{
  CDecoder *m_Decoder;
public:
  bool NeedFlush;
  CDecoderFlusher(CDecoder *decoder): m_Decoder(decoder), NeedFlush(true) {}
  ~CDecoderFlusher()
  {
    if (NeedFlush)
      m_Decoder->Flush();
    m_Decoder->ReleaseStreams();
  }
};


void CDecoder::ClearPrevLevels()
{
  unsigned i;
  for (i = 0; i < kMainTableSize; i++)
    m_LastMainLevels[i] = 0;
  for (i = 0; i < kNumLenSymbols; i++)
    m_LastLenLevels[i] = 0;
}


HRESULT CDecoder::CodeSpec(UInt32 curSize)
{
  if (_remainLen == kLenIdNeedInit)
  {
    _remainLen = 0;
    m_InBitStream.Init();
    if (!_keepHistory || !m_IsUncompressedBlock)
      m_InBitStream.Normalize();
    if (!_keepHistory)
    {
      _skipByte = false;
      m_UnCompressedBlockSize = 0;
      ClearPrevLevels();
      UInt32 i86TranslationSize = 12000000;
      bool translationMode = true;
      if (!_wimMode)
      {
        translationMode = (ReadBits(1) != 0);
        if (translationMode)
        {
          i86TranslationSize = ReadBits(16) << 16;
          i86TranslationSize |= ReadBits(16);
        }
      }
      m_x86ConvertOutStreamSpec->Init(translationMode, i86TranslationSize);
      
      for (unsigned i = 0 ; i < kNumRepDistances; i++)
        m_RepDistances[i] = 0;
    }
  }

  while (_remainLen > 0 && curSize > 0)
  {
    m_OutWindowStream.PutByte(m_OutWindowStream.GetByte(m_RepDistances[0]));
    _remainLen--;
    curSize--;
  }

  while (curSize > 0)
  {
    if (m_UnCompressedBlockSize == 0)
      if (!ReadTables())
        return S_FALSE;
    UInt32 next = (Int32)MyMin(m_UnCompressedBlockSize, curSize);
    curSize -= next;
    m_UnCompressedBlockSize -= next;
    if (m_IsUncompressedBlock)
    {
      while (next > 0)
      {
        m_OutWindowStream.PutByte(m_InBitStream.DirectReadByte());
        next--;
      }
    }
    else while (next > 0)
    {
      UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number < 256)
      {
        m_OutWindowStream.PutByte((Byte)number);
        next--;
      }
      else
      {
        UInt32 posLenSlot = number - 256;
        if (posLenSlot >= m_NumPosLenSlots)
          return S_FALSE;
        UInt32 posSlot = posLenSlot / kNumLenSlots;
        UInt32 lenSlot = posLenSlot % kNumLenSlots;
        UInt32 len = kMatchMinLen + lenSlot;
        if (lenSlot == kNumLenSlots - 1)
        {
          UInt32 lenTemp = m_LenDecoder.DecodeSymbol(&m_InBitStream);
          if (lenTemp >= kNumLenSymbols)
            return S_FALSE;
          len += lenTemp;
        }
        
        if (posSlot < kNumRepDistances)
        {
          UInt32 distance = m_RepDistances[posSlot];
          m_RepDistances[posSlot] = m_RepDistances[0];
          m_RepDistances[0] = distance;
        }
        else
        {
          UInt32 distance;
          unsigned numDirectBits;
          if (posSlot < kNumPowerPosSlots)
          {
            numDirectBits = (unsigned)(posSlot >> 1) - 1;
            distance = ((2 | (posSlot & 1)) << numDirectBits);
          }
          else
          {
            numDirectBits = kNumLinearPosSlotBits;
            distance = ((posSlot - 0x22) << kNumLinearPosSlotBits);
          }

          if (m_AlignIsUsed && numDirectBits >= kNumAlignBits)
          {
            distance += (m_InBitStream.ReadBits(numDirectBits - kNumAlignBits) << kNumAlignBits);
            UInt32 alignTemp = m_AlignDecoder.DecodeSymbol(&m_InBitStream);
            if (alignTemp >= kAlignTableSize)
              return S_FALSE;
            distance += alignTemp;
          }
          else
            distance += m_InBitStream.ReadBits(numDirectBits);
          m_RepDistances[2] = m_RepDistances[1];
          m_RepDistances[1] = m_RepDistances[0];
          m_RepDistances[0] = distance - kNumRepDistances;
        }

        UInt32 locLen = len;
        if (locLen > next)
          locLen = next;

        if (!m_OutWindowStream.CopyBlock(m_RepDistances[0], locLen))
          return S_FALSE;

        len -= locLen;
        next -= locLen;
        if (len != 0)
        {
          _remainLen = (int)len;
          return S_OK;
        }
      }
    }
  }
  return S_OK;
}

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UInt64 size = *outSize;

  RINOK(SetInStream(inStream));
  m_x86ConvertOutStreamSpec->SetStream(outStream);
  m_OutWindowStream.SetStream(m_x86ConvertOutStream);
  RINOK(SetOutStreamSize(outSize));

  CDecoderFlusher flusher(this);

  const UInt64 start = m_OutWindowStream.GetProcessedSize();
  for (;;)
  {
    UInt32 curSize = 1 << 18;
    UInt64 rem = size - (m_OutWindowStream.GetProcessedSize() - start);
    if (curSize > rem)
      curSize = (UInt32)rem;
    if (curSize == 0)
      break;
    RINOK(CodeSpec(curSize));
    if (progress != NULL)
    {
      UInt64 inSize = m_InBitStream.GetProcessedSize();
      UInt64 nowPos64 = m_OutWindowStream.GetProcessedSize() - start;
      RINOK(progress->SetRatioInfo(&inSize, &nowPos64));
    }
  }
  flusher.NeedFlush = false;
  return Flush();
}

HRESULT CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream)
{
  m_InBitStream.SetStream(inStream);
  return S_OK;
}

STDMETHODIMP CDecoder::ReleaseInStream()
{
  m_InBitStream.ReleaseStream();
  return S_OK;
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize)
{
  if (outSize == NULL)
    return E_FAIL;
  _remainLen = kLenIdNeedInit;
  m_OutWindowStream.Init(_keepHistory);
  return S_OK;
}

HRESULT CDecoder::SetParams(unsigned numDictBits)
{
  if (numDictBits < kNumDictionaryBitsMin || numDictBits > kNumDictionaryBitsMax)
    return E_INVALIDARG;
  UInt32 numPosSlots;
  if (numDictBits < 20)
    numPosSlots = 30 + (numDictBits - 15) * 2;
  else if (numDictBits == 20)
    numPosSlots = 42;
  else
    numPosSlots = 50;
  m_NumPosLenSlots = numPosSlots * kNumLenSlots;
  if (!m_OutWindowStream.Create(kDictionarySizeMax))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 16))
    return E_OUTOFMEMORY;
  return S_OK;
}

}}
