// arj/Decoder2.cpp

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

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;

  if (!m_OutWindowStream.IsCreated())
  {
    try
    {
      m_OutWindowStream.Create(kHistorySize);
    }
    catch(...)
    {
      return E_OUTOFMEMORY;
    }
  }
  UINT64 aPos = 0;
  m_OutWindowStream.Init(anOutStream, false);
  m_InBitStream.Init(anInStream);
  CCoderReleaser aCoderReleaser(this);

  while(aPos < *anOutSize)
  {
    const UINT32 kStartWidth = 0;
    const UINT32 kStopWidth = 7;
    UINT32 aPower = 1 << kStartWidth;
    UINT32 aWidth;
    UINT32 aLen = 0;
    for (aWidth = kStartWidth; aWidth < kStopWidth; aWidth++)
    {
      if (m_InBitStream.ReadBits(1) == 0)
        break;
      aLen += aPower;
      aPower <<= 1;
    }
    if (aWidth != 0)
      aLen += m_InBitStream.ReadBits(aWidth);
    if (aLen == 0)
    {
      m_OutWindowStream.PutOneByte(m_InBitStream.ReadBits(8));
      aPos++;
      continue;
    }
    else
    {
      aLen = aLen - 1 + kMatchMinLen;
      const UINT32 kStartWidth = 9;
      const UINT32 kStopWidth = 13;
      UINT32 aPower = 1 << kStartWidth;
      UINT32 aWidth;
      UINT32 aDistance = 0;
      for (aWidth = kStartWidth; aWidth < kStopWidth; aWidth++)
      {
        if (m_InBitStream.ReadBits(1) == 0)
          break;
        aDistance += aPower;
        aPower <<= 1;
      }
      if (aWidth != 0)
        aDistance += m_InBitStream.ReadBits(aWidth);
      if (aDistance >= aPos)
        throw "data error";
      m_OutWindowStream.CopyBackBlock(aDistance, aLen);
        aPos += aLen;
    }
  }
  return m_OutWindowStream.Flush();
}

STDMETHODIMP CCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  catch(const NStream::CInByteReadException &exception)
  {
    return exception.Result;
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

}}}
