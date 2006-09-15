// Rar20Decoder.cpp
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver
 
#include "StdAfx.h"

#include "Rar20Decoder.h"
#include "Rar20Const.h"

namespace NCompress {
namespace NRar20 {

class CException
{
public:
  enum ECauseType
  {
    kData
  } Cause;
  CException(ECauseType cause): Cause(cause) {}
};

static const char *kNumberErrorMessage = "Number error";

static const UInt32 kHistorySize = 1 << 20;

static const int kNumStats = 11;

static const UInt32 kWindowReservSize = (1 << 22) + 256;

CDecoder::CDecoder():
  m_IsSolid(false)
{
}

void CDecoder::InitStructures()
{
  m_Predictor.Init();
  for(int i = 0; i < kNumRepDists; i++)
    m_RepDists[i] = 0;
  m_RepDistPtr = 0;
  m_LastLength = 0;
  memset(m_LastLevels, 0, kMaxTableSize);
}

#define RIF(x) { if (!(x)) return false; }

bool CDecoder::ReadTables(void)
{
  Byte levelLevels[kLevelTableSize];
  Byte newLevels[kMaxTableSize];
  m_AudioMode = (m_InBitStream.ReadBits(1) == 1);

  if (m_InBitStream.ReadBits(1) == 0)
    memset(m_LastLevels, 0, kMaxTableSize);
  int numLevels;
  if (m_AudioMode)
  {
    m_NumChannels = m_InBitStream.ReadBits(2) + 1;
    if (m_Predictor.CurrentChannel >= m_NumChannels)
      m_Predictor.CurrentChannel = 0;
    numLevels = m_NumChannels * kMMTableSize;
  }
  else
    numLevels = kHeapTablesSizesSum;
 
  int i;
  for (i = 0; i < kLevelTableSize; i++)
    levelLevels[i] = Byte(m_InBitStream.ReadBits(4));
  RIF(m_LevelDecoder.SetCodeLengths(levelLevels));
  i = 0;
  while (i < numLevels)
  {
    UInt32 number = m_LevelDecoder.DecodeSymbol(&m_InBitStream);
    if (number < kTableDirectLevels)
    {
      newLevels[i] = Byte((number + m_LastLevels[i]) & kLevelMask);
      i++;
    }
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
        else if (number == kTableLevel0Number2)
          num = m_InBitStream.ReadBits(7) + 11;
        else 
          return false;
        for (;num > 0 && i < numLevels; num--)
          newLevels[i++] = 0;
      }
    }
  }
  if (m_AudioMode)
    for (i = 0; i < m_NumChannels; i++)
    {
      RIF(m_MMDecoders[i].SetCodeLengths(&newLevels[i * kMMTableSize]));
    }
  else
  {
    RIF(m_MainDecoder.SetCodeLengths(&newLevels[0]));
    RIF(m_DistDecoder.SetCodeLengths(&newLevels[kMainTableSize]));
    RIF(m_LenDecoder.SetCodeLengths(&newLevels[kMainTableSize + kDistTableSize]));
  }
  memcpy(m_LastLevels, newLevels, kMaxTableSize);
  return true;
}

bool CDecoder::ReadLastTables()
{
  // it differs a little from pure RAR sources;
  // UInt64 ttt = m_InBitStream.GetProcessedSize() + 2;
  // + 2 works for: return 0xFF; in CInBuffer::ReadByte.
  if (m_InBitStream.GetProcessedSize() + 7 <= m_PackSize) // test it: probably incorrect; 
  // if (m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect; 
    if (m_AudioMode)
    {
      UInt32 symbol = m_MMDecoders[m_Predictor.CurrentChannel].DecodeSymbol(&m_InBitStream);
      if (symbol == 256)
        return ReadTables();
      if (symbol >= kMMTableSize)
        return false;
    }
    else 
    {
      UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
      if (number == kReadTableNumber)
        return ReadTables();
      if (number >= kMainTableSize)
        return false;
    }
  return true;
}

class CCoderReleaser
{
  CDecoder *m_Coder;
public:
  CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
  ~CCoderReleaser()
  {
    m_Coder->ReleaseStreams();
  }
};

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (inSize == NULL || outSize == NULL)
    return E_INVALIDARG;

  if (!m_OutWindowStream.Create(kHistorySize))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  m_PackSize = *inSize;

  UInt64 pos = 0, unPackSize = *outSize;
  
  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(m_IsSolid);
  m_InBitStream.SetStream(inStream);
  m_InBitStream.Init();

  CCoderReleaser coderReleaser(this);
  if (!m_IsSolid)
  {
    InitStructures();
    if (unPackSize == 0)
    {
      if (m_InBitStream.GetProcessedSize() + 2 <= m_PackSize) // test it: probably incorrect; 
        if (!ReadTables())
          return S_FALSE;
      return S_OK;
    }
    if (!ReadTables())
      return S_FALSE;
  }

  while(pos < unPackSize)
  {
    if (m_AudioMode) 
      while(pos < unPackSize)
      {
        UInt32 symbol = m_MMDecoders[m_Predictor.CurrentChannel].DecodeSymbol(&m_InBitStream);
        if (symbol == 256)
        {
          if (progress != 0)
          {
            UInt64 packSize = m_InBitStream.GetProcessedSize();
            RINOK(progress->SetRatioInfo(&packSize, &pos));
          }
          if (!ReadTables())
            return S_FALSE;
          break;
        }
        if (symbol >= kMMTableSize)
          return S_FALSE;
        Byte byPredict = m_Predictor.Predict();
        Byte byReal = (Byte)(byPredict - (Byte)symbol);
        m_Predictor.Update(byReal, byPredict);
        m_OutWindowStream.PutByte(byReal);
        if (++m_Predictor.CurrentChannel == m_NumChannels)
          m_Predictor.CurrentChannel = 0;
        pos++;
      }
    else
      while(pos < unPackSize)
      {
        UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
        UInt32 length, distance;
        if (number < 256)
        {
          m_OutWindowStream.PutByte(Byte(number));
          pos++;
          continue;
        }
        else if (number >= kMatchNumber)
        {
          number -= kMatchNumber;
          length = kNormalMatchMinLen + UInt32(kLenStart[number]) + 
              m_InBitStream.ReadBits(kLenDirectBits[number]);
          number = m_DistDecoder.DecodeSymbol(&m_InBitStream);
          if (number >= kDistTableSize)
            return S_FALSE;
          distance = kDistStart[number] + m_InBitStream.ReadBits(kDistDirectBits[number]);
          if (distance >= kDistLimit3)
          {
            length += 2 - ((distance - kDistLimit4) >> 31);
            // length++;
            // if (distance >= kDistLimit4)
            //  length++;
          }
        }
        else if (number == kRepBothNumber)
        {
          length = m_LastLength;
          distance = m_RepDists[(m_RepDistPtr + 4 - 1) & 3];
        }
        else if (number < kLen2Number)
        {
          distance = m_RepDists[(m_RepDistPtr - (number - kRepNumber + 1)) & 3];
          number = m_LenDecoder.DecodeSymbol(&m_InBitStream);
          if (number >= kLenTableSize)
            return S_FALSE;
          length = 2 + kLenStart[number] + m_InBitStream.ReadBits(kLenDirectBits[number]);
          if (distance >= kDistLimit2)
          {
            length++;
            if (distance >= kDistLimit3)
            {
              length += 2 - ((distance - kDistLimit4) >> 31);
              // length++;
              // if (distance >= kDistLimit4)
              //   length++;
            }
          }
        }
        else if (number < kReadTableNumber)
        {
          number -= kLen2Number;
          distance = kLen2DistStarts[number] + 
              m_InBitStream.ReadBits(kLen2DistDirectBits[number]);
          length = 2;
        }
        else if (number == kReadTableNumber)
        {
          if (progress != 0)
          {
            UInt64 packSize = m_InBitStream.GetProcessedSize();
            RINOK(progress->SetRatioInfo(&packSize, &pos));
          }
          if (!ReadTables())
            return S_FALSE;
          break;
        }
        else
          return S_FALSE;
        m_RepDists[m_RepDistPtr++ & 3] = distance;
        m_LastLength = length;
        if (!m_OutWindowStream.CopyBlock(distance, length))
          return S_FALSE;
        pos += length;
      }
  }
  if (pos > unPackSize)
    throw CException(CException::kData);

  if (!ReadLastTables())
    return S_FALSE;
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(const CLZOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size)
{
  if (size < 1)
    return E_INVALIDARG;
  m_IsSolid = (data[0] != 0);
  return S_OK;
}

}}
