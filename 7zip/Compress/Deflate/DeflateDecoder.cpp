// DeflateDecoder.cpp

#include "StdAfx.h"

#include "DeflateDecoder.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

const int kLenIdFinished = -1;
const int kLenIdNeedInit = -2;

CCoder::CCoder(bool deflate64Mode):  _deflate64Mode(deflate64Mode), _keepHistory(false) {}

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

#define RIF(x) { if (!(x)) return false; }

bool CCoder::ReadTables(void)
{
  m_FinalBlock = (m_InBitStream.ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);
  int blockType = m_InBitStream.ReadBits(kBlockTypeFieldSize);
  if (blockType > NBlockType::kDynamicHuffman)
    return false;

  if (blockType == NBlockType::kStored)
  {
    m_StoredMode = true;
    UInt32 currentBitPosition = m_InBitStream.GetBitPosition();
    UInt32 numBitsForAlign = currentBitPosition > 0 ? (8 - currentBitPosition): 0;
    if (numBitsForAlign > 0)
      m_InBitStream.ReadBits(numBitsForAlign);
    m_StoredBlockSize = m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize);
    UInt16 onesComplementReverse = ~(UInt16)(m_InBitStream.ReadBits(kDeflateStoredBlockLengthFieldSizeSize));
    return (m_StoredBlockSize == onesComplementReverse);
  }

  m_StoredMode = false;
  Byte litLenLevels[kStaticMainTableSize];
  Byte distLevels[kStaticDistTableSize];
  if (blockType == NBlockType::kFixedHuffman)
  {
    int i;
    
    for (i = 0; i < 144; i++)
      litLenLevels[i] = 8;
    for (; i < 256; i++)
      litLenLevels[i] = 9;
    for (; i < 280; i++)
      litLenLevels[i] = 7;
    for (; i < 288; i++)          // make a complete, but wrong code set
      litLenLevels[i] = 8;
    
    for (i = 0; i < kStaticDistTableSize; i++)  // test it: InfoZip only uses kDistTableSize       
      distLevels[i] = 5;
  }
  else // (blockType == kDynamicHuffman)
  {
    int numLitLenLevels = m_InBitStream.ReadBits(kDeflateNumberOfLengthCodesFieldSize) + 
        kDeflateNumberOfLitLenCodesMin;
    int numDistLevels = m_InBitStream.ReadBits(kDeflateNumberOfDistanceCodesFieldSize) + 
        kDeflateNumberOfDistanceCodesMin;
    int numLevelCodes = m_InBitStream.ReadBits(kDeflateNumberOfLevelCodesFieldSize) + 
        kDeflateNumberOfLevelCodesMin;
    
    int numLevels = _deflate64Mode ? kHeapTablesSizesSum64 : kHeapTablesSizesSum32;
    
    Byte levelLevels[kLevelTableSize];
    for (int i = 0; i < kLevelTableSize; i++)
    {
      int position = kCodeLengthAlphabetOrder[i]; 
      if(i < numLevelCodes)
        levelLevels[position] = Byte(m_InBitStream.ReadBits(kDeflateLevelCodeFieldSize));
      else
        levelLevels[position] = 0;
    }
    
    RIF(m_LevelDecoder.SetCodeLengths(levelLevels));
    
    Byte tmpLevels[kStaticMaxTableSize];
    DeCodeLevelTable(tmpLevels, numLitLenLevels + numDistLevels);
    
    memmove(litLenLevels, tmpLevels, numLitLenLevels);
    memset(litLenLevels + numLitLenLevels, 0, kStaticMainTableSize - numLitLenLevels);
    
    memmove(distLevels, tmpLevels + numLitLenLevels, numDistLevels);
    memset(distLevels + numDistLevels, 0, kStaticDistTableSize - numDistLevels);
  }
  RIF(m_MainDecoder.SetCodeLengths(litLenLevels));
  return m_DistDecoder.SetCodeLengths(distLevels);
}

HRESULT CCoder::CodeSpec(UInt32 curSize)
{
  if (_remainLen == kLenIdFinished)
    return S_OK;
  if (_remainLen == kLenIdNeedInit)
  {
    if (!_keepHistory)
    {
      if (!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64:  kHistorySize32))
        return E_OUTOFMEMORY;
    }
    if (!m_InBitStream.Create(1 << 17))
      return E_OUTOFMEMORY;
    m_OutWindowStream.Init(_keepHistory);
    m_InBitStream.Init();
    m_FinalBlock = false;
    _remainLen = 0;
    _needReadTable = true;
  }

  if (curSize == 0)
    return S_OK;

  while(_remainLen > 0 && curSize > 0)
  {
    _remainLen--;
    Byte b = m_OutWindowStream.GetByte(_rep0);
    m_OutWindowStream.PutByte(b);
    curSize--;
  }

  while(curSize > 0)
  {
    if (_needReadTable)
    {
      if (m_FinalBlock)
      {
        _remainLen = kLenIdFinished;
        break;
      }
      if (!ReadTables())
        return S_FALSE;
      _needReadTable = false;
    }

    if(m_StoredMode)
    {
      for (; m_StoredBlockSize > 0 && curSize > 0; m_StoredBlockSize--, curSize--)
        m_OutWindowStream.PutByte((Byte)m_InBitStream.ReadBits(8));
      _needReadTable = (m_StoredBlockSize == 0);
      continue;
    }
    while(curSize > 0)
    {
      if (m_InBitStream.NumExtraBytes > 4)
        return S_FALSE;

      UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number < 256)
      {
        m_OutWindowStream.PutByte((Byte)number);
        curSize--;
        continue;
      }
      else if (number >= kMatchNumber)
      {
        number -= kMatchNumber;
        UInt32 len;
        {
          int numBits;
          if (_deflate64Mode)
          {
            len = kLenStart64[number];
            numBits = kLenDirectBits64[number];
          }
          else
          {
            len = kLenStart32[number];
            numBits = kLenDirectBits32[number];
          }
          len += kMatchMinLen + m_InBitStream.ReadBits(numBits);
        }
        UInt32 locLen = len;
        if (locLen > curSize)
          locLen = (UInt32)curSize;
        number = m_DistDecoder.DecodeSymbol(&m_InBitStream);
        if (number >= kStaticDistTableSize)
          return S_FALSE;
        UInt32 distance = kDistStart[number] + m_InBitStream.ReadBits(kDistDirectBits[number]);
        if (!m_OutWindowStream.CopyBlock(distance, locLen))
          return S_FALSE;
        curSize -= locLen;
        len -= locLen;
        if (len != 0)
        {
          _remainLen = (int)len;
          _rep0 = distance;
          break;
        }
      }
      else if (number == kReadTableNumber)
      {
        _needReadTable = true;
        break;
      }
      else
        return S_FALSE;
    }
  }
  return S_OK;
}

HRESULT CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UInt64 *, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  SetInStream(inStream);
  m_OutWindowStream.SetStream(outStream);
  SetOutStreamSize(outSize);
  CCoderReleaser flusher(this);

  const UInt64 start = m_OutWindowStream.GetProcessedSize();
  while(true)
  {
    UInt32 curSize = 1 << 18;
    if (outSize != 0)
    {
      const UInt64 rem = *outSize - (m_OutWindowStream.GetProcessedSize() - start);
      if (curSize > rem)
        curSize = (UInt32)rem;
    }
    if (curSize == 0)
      break;
    RINOK(CodeSpec(curSize));
    if (_remainLen == kLenIdFinished)
      break;
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


#ifdef _NO_EXCEPTIONS

#define DEFLATE_TRY_BEGIN
#define DEFLATE_TRY_END

#else

#define DEFLATE_TRY_BEGIN try { 
#define DEFLATE_TRY_END } \
  catch(const CInBufferException &e)  { return e.ErrorCode; } \
  catch(const CLZOutWindowException &e)  { return e.ErrorCode; } \
  catch(...) { return S_FALSE; }

#endif

HRESULT CCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  DEFLATE_TRY_BEGIN
  return CodeReal(inStream, outStream, inSize, outSize, progress);
  DEFLATE_TRY_END
}

STDMETHODIMP CCoder::GetInStreamProcessedSize(UInt64 *value)
{
  if (value == NULL)
    return E_INVALIDARG;
  *value = m_InBitStream.GetProcessedSize();
  return S_OK;
}

STDMETHODIMP CCoder::SetInStream(ISequentialInStream *inStream)
{
  m_InBitStream.SetStream(inStream);
  return S_OK;
}

STDMETHODIMP CCoder::ReleaseInStream()
{
  m_InBitStream.ReleaseStream();
  return S_OK;
}

STDMETHODIMP CCoder::SetOutStreamSize(const UInt64 *outSize)
{
  _remainLen = kLenIdNeedInit;
  m_OutWindowStream.Init(_keepHistory);
  return S_OK;
}

#ifdef _ST_MODE

STDMETHODIMP CCoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  DEFLATE_TRY_BEGIN
  if (processedSize)
    *processedSize = 0;
  const UInt64 startPos = m_OutWindowStream.GetProcessedSize();
  m_OutWindowStream.SetMemStream((Byte *)data);
  RINOK(CodeSpec(size));
  if (processedSize)
    *processedSize = (UInt32)(m_OutWindowStream.GetProcessedSize() - startPos);
  return Flush();
  DEFLATE_TRY_END
}

#endif

}}}
