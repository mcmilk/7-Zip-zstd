// Arj/Decoder2.cpp

#include "StdAfx.h"

#include "Decoder2.h"

#include "Windows/Defs.h"

namespace NCompress{
namespace NArj {
namespace NDecoder2 {

static const UINT32 kHistorySize = 26624;
static const UINT32 kMatchMaxLen = 256;
static const UINT32 kMatchMinLen = 3;

CCoder::CCoder()
{}

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;

  if (!m_OutWindowStream.IsCreated())
  {
    try { m_OutWindowStream.Create(kHistorySize); }
    catch(...) { return E_OUTOFMEMORY; }
  }
  UINT64 pos = 0;
  m_OutWindowStream.Init(outStream, false);
  m_InBitStream.Init(inStream);
  CCoderReleaser coderReleaser(this);

  while(pos < *outSize)
  {
    const UINT32 kStartWidth = 0;
    const UINT32 kStopWidth = 7;
    UINT32 power = 1 << kStartWidth;
    UINT32 width;
    UINT32 len = 0;
    for (width = kStartWidth; width < kStopWidth; width++)
    {
      if (m_InBitStream.ReadBits(1) == 0)
        break;
      len += power;
      power <<= 1;
    }
    if (width != 0)
      len += m_InBitStream.ReadBits(width);
    if (len == 0)
    {
      m_OutWindowStream.PutOneByte(m_InBitStream.ReadBits(8));
      pos++;
      continue;
    }
    else
    {
      len = len - 1 + kMatchMinLen;
      const UINT32 kStartWidth = 9;
      const UINT32 kStopWidth = 13;
      UINT32 power = 1 << kStartWidth;
      UINT32 width;
      UINT32 distance = 0;
      for (width = kStartWidth; width < kStopWidth; width++)
      {
        if (m_InBitStream.ReadBits(1) == 0)
          break;
        distance += power;
        power <<= 1;
      }
      if (width != 0)
        distance += m_InBitStream.ReadBits(width);
      if (distance >= pos)
        throw "data error";
      m_OutWindowStream.CopyBackBlock(distance, len);
        pos += len;
    }
  }
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);}
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLZOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}}
