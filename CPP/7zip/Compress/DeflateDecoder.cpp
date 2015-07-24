// DeflateDecoder.cpp

#include "StdAfx.h"

#include "DeflateDecoder.h"

namespace NCompress {
namespace NDeflate {
namespace NDecoder {

CCoder::CCoder(bool deflate64Mode, bool deflateNSIS):
    _deflate64Mode(deflate64Mode),
    _deflateNSIS(deflateNSIS),
    _keepHistory(false),
    _needFinishInput(false),
    _needInitInStream(true),
    ZlibMode(false) {}

UInt32 CCoder::ReadBits(unsigned numBits)
{
  return m_InBitStream.ReadBits(numBits);
}

Byte CCoder::ReadAlignedByte()
{
  return m_InBitStream.ReadAlignedByte();
}

bool CCoder::DeCodeLevelTable(Byte *values, unsigned numSymbols)
{
  unsigned i = 0;
  do
  {
    UInt32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number < kTableDirectLevels)
      values[i++] = (Byte)number;
    else if (number < kLevelTableSize)
    {
      if (number == kTableLevelRepNumber)
      {
        if (i == 0)
          return false;
        unsigned num = ReadBits(2) + 3;
        for (; num > 0 && i < numSymbols; num--, i++)
          values[i] = values[i - 1];
      }
      else
      {
        unsigned num;
        if (number == kTableLevel0Number)
          num = ReadBits(3) + 3;
        else
          num = ReadBits(7) + 11;
        for (; num > 0 && i < numSymbols; num--)
          values[i++] = 0;
      }
    }
    else
      return false;
  }
  while (i < numSymbols);
  return true;
}

#define RIF(x) { if (!(x)) return false; }

bool CCoder::ReadTables(void)
{
  m_FinalBlock = (ReadBits(kFinalBlockFieldSize) == NFinalBlockField::kFinalBlock);
  if (m_InBitStream.ExtraBitsWereRead())
    return false;
  UInt32 blockType = ReadBits(kBlockTypeFieldSize);
  if (blockType > NBlockType::kDynamicHuffman)
    return false;
  if (m_InBitStream.ExtraBitsWereRead())
    return false;

  if (blockType == NBlockType::kStored)
  {
    m_StoredMode = true;
    m_InBitStream.AlignToByte();
    m_StoredBlockSize = ReadAligned_UInt16(); // ReadBits(kStoredBlockLengthFieldSize)
    if (_deflateNSIS)
      return true;
    return (m_StoredBlockSize == (UInt16)~ReadAligned_UInt16());
  }

  m_StoredMode = false;

  CLevels levels;
  if (blockType == NBlockType::kFixedHuffman)
  {
    levels.SetFixedLevels();
    _numDistLevels = _deflate64Mode ? kDistTableSize64 : kDistTableSize32;
  }
  else
  {
    unsigned numLitLenLevels = ReadBits(kNumLenCodesFieldSize) + kNumLitLenCodesMin;
    _numDistLevels = ReadBits(kNumDistCodesFieldSize) + kNumDistCodesMin;
    unsigned numLevelCodes = ReadBits(kNumLevelCodesFieldSize) + kNumLevelCodesMin;

    if (!_deflate64Mode)
      if (_numDistLevels > kDistTableSize32)
        return false;
    
    Byte levelLevels[kLevelTableSize];
    for (unsigned i = 0; i < kLevelTableSize; i++)
    {
      unsigned position = kCodeLengthAlphabetOrder[i];
      if (i < numLevelCodes)
        levelLevels[position] = (Byte)ReadBits(kLevelFieldSize);
      else
        levelLevels[position] = 0;
    }
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    RIF(m_LevelDecoder.SetCodeLengths(levelLevels));
    
    Byte tmpLevels[kFixedMainTableSize + kFixedDistTableSize];
    if (!DeCodeLevelTable(tmpLevels, numLitLenLevels + _numDistLevels))
      return false;
    
    if (m_InBitStream.ExtraBitsWereRead())
      return false;

    levels.SubClear();
    memcpy(levels.litLenLevels, tmpLevels, numLitLenLevels);
    memcpy(levels.distLevels, tmpLevels + numLitLenLevels, _numDistLevels);
  }
  RIF(m_MainDecoder.SetCodeLengths(levels.litLenLevels));
  return m_DistDecoder.SetCodeLengths(levels.distLevels);
}

HRESULT CCoder::CodeSpec(UInt32 curSize, bool finishInputStream)
{
  if (_remainLen == kLenIdFinished)
    return S_OK;
  if (_remainLen == kLenIdNeedInit)
  {
    if (!_keepHistory)
      if (!m_OutWindowStream.Create(_deflate64Mode ? kHistorySize64: kHistorySize32))
        return E_OUTOFMEMORY;
    RINOK(InitInStream(_needInitInStream));
    m_OutWindowStream.Init(_keepHistory);
    m_FinalBlock = false;
    _remainLen = 0;
    _needReadTable = true;
  }

  while (_remainLen > 0 && curSize > 0)
  {
    _remainLen--;
    Byte b = m_OutWindowStream.GetByte(_rep0);
    m_OutWindowStream.PutByte(b);
    curSize--;
  }

  while (curSize > 0 || finishInputStream)
  {
    if (m_InBitStream.ExtraBitsWereRead())
      return S_FALSE;
    if (_needReadTable)
    {
      if (m_FinalBlock)
      {
        _remainLen = kLenIdFinished;
        break;
      }
      if (!ReadTables())
        return S_FALSE;
      if (m_InBitStream.ExtraBitsWereRead())
        return S_FALSE;
      _needReadTable = false;
    }

    if (m_StoredMode)
    {
      if (finishInputStream && curSize == 0 && m_StoredBlockSize != 0)
        return S_FALSE;
      /* NSIS version contains some bits in bitl bits buffer.
         So we must read some first bytes via ReadAlignedByte */
      for (; m_StoredBlockSize > 0 && curSize > 0 && m_InBitStream.ThereAreDataInBitsBuffer(); m_StoredBlockSize--, curSize--)
        m_OutWindowStream.PutByte(ReadAlignedByte());
      for (; m_StoredBlockSize > 0 && curSize > 0; m_StoredBlockSize--, curSize--)
        m_OutWindowStream.PutByte(m_InBitStream.ReadDirectByte());
      _needReadTable = (m_StoredBlockSize == 0);
      continue;
    }
    
    while (curSize > 0)
    {
      if (m_InBitStream.ExtraBitsWereRead_Fast())
        return S_FALSE;

      UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number < 0x100)
      {
        m_OutWindowStream.PutByte((Byte)number);
        curSize--;
        continue;
      }
      else if (number == kSymbolEndOfBlock)
      {
        _needReadTable = true;
        break;
      }
      else if (number < kMainTableSize)
      {
        number -= kSymbolMatch;
        UInt32 len;
        {
          unsigned numBits;
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
        if (number >= _numDistLevels)
          return S_FALSE;
        UInt32 distance = kDistStart[number] + m_InBitStream.ReadBits(kDistDirectBits[number]);
        if (!m_OutWindowStream.CopyBlock(distance, locLen))
          return S_FALSE;
        curSize -= locLen;
        len -= locLen;
        if (len != 0)
        {
          _remainLen = (Int32)len;
          _rep0 = distance;
          break;
        }
      }
      else
        return S_FALSE;
    }
    
    if (finishInputStream && curSize == 0)
    {
      if (m_MainDecoder.DecodeSymbol(&m_InBitStream) != kSymbolEndOfBlock)
        return S_FALSE;
      _needReadTable = true;
    }
  }

  if (m_InBitStream.ExtraBitsWereRead())
    return S_FALSE;

  return S_OK;
}

#ifdef _NO_EXCEPTIONS

#define DEFLATE_TRY_BEGIN
#define DEFLATE_TRY_END(res)

#else

#define DEFLATE_TRY_BEGIN try {
#define DEFLATE_TRY_END(res) } \
  catch(const CInBufferException &e)  { res = e.ErrorCode; } \
  catch(const CLzOutWindowException &e)  { res = e.ErrorCode; } \
  catch(...) { res = S_FALSE; }

#endif

HRESULT CCoder::CodeReal(ISequentialOutStream *outStream,
      const UInt64 *outSize, ICompressProgressInfo *progress)
{
  HRESULT res;
  DEFLATE_TRY_BEGIN
  m_OutWindowStream.SetStream(outStream);
  CCoderReleaser flusher(this);

  const UInt64 inStart = _needInitInStream ? 0 : m_InBitStream.GetProcessedSize();
  const UInt64 start = m_OutWindowStream.GetProcessedSize();
  for (;;)
  {
    UInt32 curSize = 1 << 18;
    bool finishInputStream = false;
    if (outSize)
    {
      const UInt64 rem = *outSize - (m_OutWindowStream.GetProcessedSize() - start);
      if (curSize >= rem)
      {
        curSize = (UInt32)rem;
        if (ZlibMode || _needFinishInput)
          finishInputStream = true;
      }
    }
    if (!finishInputStream && curSize == 0)
      break;
    RINOK(CodeSpec(curSize, finishInputStream));
    if (_remainLen == kLenIdFinished)
      break;
    if (progress)
    {
      const UInt64 inSize = m_InBitStream.GetProcessedSize() - inStart;
      const UInt64 nowPos64 = m_OutWindowStream.GetProcessedSize() - start;
      RINOK(progress->SetRatioInfo(&inSize, &nowPos64));
    }
  }
  if (_remainLen == kLenIdFinished && ZlibMode)
  {
    m_InBitStream.AlignToByte();
    for (unsigned i = 0; i < 4; i++)
      ZlibFooter[i] = ReadAlignedByte();
  }
  flusher.NeedFlush = false;
  res = Flush();
  if (res == S_OK && _remainLen != kLenIdNeedInit && InputEofError())
    return S_FALSE;
  DEFLATE_TRY_END(res)
  return res;
}

HRESULT CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetInStream(inStream);
  SetOutStreamSize(outSize);
  HRESULT res = CodeReal(outStream, outSize, progress);
  ReleaseInStream();
  return res;
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
  m_InStreamRef = inStream;
  m_InBitStream.SetStream(inStream);
  return S_OK;
}

STDMETHODIMP CCoder::ReleaseInStream()
{
  m_InStreamRef.Release();
  return S_OK;
}

STDMETHODIMP CCoder::SetOutStreamSize(const UInt64 * /* outSize */)
{
  _remainLen = kLenIdNeedInit;
  _needInitInStream = true;
  m_OutWindowStream.Init(_keepHistory);
  return S_OK;
}

#ifndef NO_READ_FROM_CODER

STDMETHODIMP CCoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT res;
  DEFLATE_TRY_BEGIN
  if (processedSize)
    *processedSize = 0;
  const UInt64 startPos = m_OutWindowStream.GetProcessedSize();
  m_OutWindowStream.SetMemStream((Byte *)data);
  res = CodeSpec(size, false);
  if (res == S_OK)
  {
    res = Flush();
    if (processedSize)
      *processedSize = (UInt32)(m_OutWindowStream.GetProcessedSize() - startPos);
  }
  DEFLATE_TRY_END(res)
  m_OutWindowStream.SetMemStream(NULL);
  return res;
}

#endif

STDMETHODIMP CCoder::CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  _remainLen = kLenIdNeedInit;
  m_OutWindowStream.Init(_keepHistory);
  return CodeReal(outStream, outSize, progress);
}

}}}
