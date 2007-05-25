// LzhDecoder.cpp

#include "StdAfx.h"

#include "LzhDecoder.h"

#include "Windows/Defs.h"

namespace NCompress{
namespace NLzh {
namespace NDecoder {

static const UInt32 kHistorySize = (1 << 16);

static const int kBlockSizeBits = 16;
static const int kNumCBits = 9;
static const int kNumLevelBits = 5; // smallest integer such that (1 << kNumLevelBits) > kNumLevelSymbols/

UInt32 CCoder::ReadBits(int numBits) {  return m_InBitStream.ReadBits(numBits); }

HRESULT CCoder::ReadLevelTable()
{
  int n = ReadBits(kNumLevelBits);
  if (n == 0) 
  {
    m_LevelHuffman.Symbol = ReadBits(kNumLevelBits);
    if (m_LevelHuffman.Symbol >= kNumLevelSymbols)
      return S_FALSE;
  }
  else 
  {
    if (n > kNumLevelSymbols)
      return S_FALSE;
    m_LevelHuffman.Symbol = -1;
    Byte lens[kNumLevelSymbols];
    int i = 0;
    while (i < n) 
    {
      int c = m_InBitStream.ReadBits(3);
      if (c == 7) 
        while (ReadBits(1)) 
          if (c++ > kMaxHuffmanLen)
            return S_FALSE;
      lens[i++] = (Byte)c;
      if (i == kNumSpecLevelSymbols) 
      {
        c = ReadBits(2);
        while (--c >= 0)
          lens[i++] = 0;
      }
    }
    while (i < kNumLevelSymbols)
      lens[i++] = 0;
    m_LevelHuffman.SetCodeLengths(lens);
  }
  return S_OK;
}

HRESULT CCoder::ReadPTable(int numBits)
{
  int n = ReadBits(numBits);
  if (n == 0)
  {
    m_PHuffmanDecoder.Symbol = ReadBits(numBits);
    if (m_PHuffmanDecoder.Symbol >= kNumDistanceSymbols)
      return S_FALSE;
  }
  else 
  {
    if (n > kNumDistanceSymbols)
      return S_FALSE;
    m_PHuffmanDecoder.Symbol = -1;
    Byte lens[kNumDistanceSymbols];
    int i = 0;
    while (i < n) 
    {
      int c = m_InBitStream.ReadBits(3);
      if (c == 7) 
        while (ReadBits(1)) 
        {
          if (c > kMaxHuffmanLen)
            return S_FALSE;
          c++;
        }
      lens[i++] = (Byte)c;
    }
    while (i < kNumDistanceSymbols)
      lens[i++] = 0;
    m_PHuffmanDecoder.SetCodeLengths(lens);
  }
  return S_OK;
}

HRESULT CCoder::ReadCTable()
{
  int n = ReadBits(kNumCBits);
  if (n == 0) 
  {
    m_CHuffmanDecoder.Symbol = ReadBits(kNumCBits);
    if (m_CHuffmanDecoder.Symbol >= kNumCSymbols)
      return S_FALSE;
  }
  else 
  {
    if (n > kNumCSymbols)
      return S_FALSE;
    m_CHuffmanDecoder.Symbol = -1;
    Byte lens[kNumCSymbols];
    int i = 0;
    while (i < n) 
    {
      int c = m_LevelHuffman.Decode(&m_InBitStream);
      if (c < kNumSpecLevelSymbols) 
      {
        if (c == 0)
          c = 1;
        else if (c == 1)
          c = ReadBits(4) + 3;
        else
          c = ReadBits(kNumCBits) + 20;
        while (--c >= 0)
        {
          if (i > kNumCSymbols)
            return S_FALSE;
          lens[i++] = 0;
        }
      }
      else
        lens[i++] = (Byte)(c - 2);
    }
    while (i < kNumCSymbols)
      lens[i++] = 0;
    m_CHuffmanDecoder.SetCodeLengths(lens);
  }
  return S_OK;
}

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 * /* inSize */, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;

  if (!m_OutWindowStream.Create(kHistorySize))
    return E_OUTOFMEMORY;
  if (!m_InBitStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  UInt64 pos = 0;
  m_OutWindowStream.SetStream(outStream);
  m_OutWindowStream.Init(false);
  m_InBitStream.SetStream(inStream);
  m_InBitStream.Init();
  
  CCoderReleaser coderReleaser(this);

  int pbit;
  if (m_NumDictBits <= 13)  
    pbit = 4;
  else   
    pbit = 5;

  UInt32 blockSize = 0;

  while(pos < *outSize)
  {
    // for (i = 0; i < dictSize; i++) dtext[i] = 0x20;
    
    if (blockSize == 0) 
    {
      if (progress != NULL)
      {
        UInt64 packSize = m_InBitStream.GetProcessedSize();
        RINOK(progress->SetRatioInfo(&packSize, &pos));
      }
      blockSize = ReadBits(kBlockSizeBits);
      ReadLevelTable();
      ReadCTable();
      RINOK(ReadPTable(pbit));
    }
    blockSize--;
    UInt32 c = m_CHuffmanDecoder.Decode(&m_InBitStream);
    if (c < 256) 
    {
      m_OutWindowStream.PutByte((Byte)c);
      pos++;
    }
    else if (c >= kNumCSymbols)
      return S_FALSE;
    else
    {
      // offset = (interface->method == LARC_METHOD_NUM) ? 0x100 - 2 : 0x100 - 3;
      UInt32 len  = c - 256 + kMinMatch;
      UInt32 distance = m_PHuffmanDecoder.Decode(&m_InBitStream);
      if (distance != 0)
        distance = (1 << (distance - 1)) + ReadBits(distance - 1);
      if (distance >= pos)
        return S_FALSE;
      if (pos + len > *outSize)
        len = (UInt32)(*outSize - pos);
      pos += len;
      m_OutWindowStream.CopyBlock(distance, len);
    }
  }
  coderReleaser.NeedFlush = false;
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);}
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLZOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}}
