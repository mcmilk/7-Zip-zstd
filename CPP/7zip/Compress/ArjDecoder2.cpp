// ArjDecoder2.cpp

#include "StdAfx.h"

#include "ArjDecoder2.h"

namespace NCompress{
namespace NArj {
namespace NDecoder2 {

static const UInt32 kHistorySize = 26624;
static const UInt32 kMatchMinLen = 3;

HRESULT CCoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo * /* progress */)
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

  while(pos < *outSize)
  {
    const UInt32 kStartWidth = 0;
    const UInt32 kStopWidth = 7;
    UInt32 power = 1 << kStartWidth;
    UInt32 width;
    UInt32 len = 0;
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
      m_OutWindowStream.PutByte((Byte)m_InBitStream.ReadBits(8));
      pos++;
      continue;
    }
    else
    {
      len = len - 1 + kMatchMinLen;
      const UInt32 kStartWidth = 9;
      const UInt32 kStopWidth = 13;
      UInt32 power = 1 << kStartWidth;
      UInt32 width;
      UInt32 distance = 0;
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
        return S_FALSE;
      m_OutWindowStream.CopyBlock(distance, len);
        pos += len;
    }
  }
  coderReleaser.NeedFlush = false;
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress);}
  catch(const CInBufferException &e) { return e.ErrorCode; }
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}}}
