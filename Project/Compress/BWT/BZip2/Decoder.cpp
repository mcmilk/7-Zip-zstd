// Deflate/Decoder.cpp

#include "StdAfx.h"

#include "Decoder.h"

#include "Windows/Defs.h"
#include "Alien/Compress/BWT/BZip2/bzlib.h"

namespace NCompress {
namespace NBZip2 {
namespace NDecoder {

static const UINT32 kBufferSize = (1 << 20);

CCoder::CCoder()
{
  m_InBuffer = new BYTE[kBufferSize];
  m_OutBuffer = new BYTE[kBufferSize];
}

CCoder::~CCoder()
{
  delete []m_OutBuffer;
  delete []m_InBuffer;
}

struct CBZip2Decompressor: public bz_stream
{
  bz_stream m_Object;
public:
  int Init(int aVerbosity, int aSmall) { return BZ2_bzDecompressInit(this, aVerbosity, aSmall); }
  int Decompress()  { return BZ2_bzDecompress(this); }
  int End()  { return BZ2_bzDecompressEnd(this); }
  UINT64 GetTotalIn() const { return (UINT64(total_in_hi32) << 32) +  total_in_lo32; }
  UINT64 GetTotalOut() const { return (UINT64(total_out_hi32) << 32) +  total_out_lo32; }
};

class CBZip2DecompressorReleaser
{
  CBZip2Decompressor *m_Decompressor;
public:
  CBZip2DecompressorReleaser(CBZip2Decompressor *aDecompressor): m_Decompressor(aDecompressor) {}
  void Diable() { m_Decompressor = NULL; }
  ~CBZip2DecompressorReleaser()  { if (m_Decompressor != NULL) m_Decompressor->End(); }
};

STDMETHODIMP CCoder::CodeReal(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  CBZip2Decompressor aBZStream;
  aBZStream.bzalloc = NULL;
  aBZStream.bzfree = NULL;
  aBZStream.opaque = NULL;


  int aResult = aBZStream.Init(0, 0);
  switch(aResult)
  {
    case BZ_OK:
      break;
    case BZ_MEM_ERROR:
      return E_OUTOFMEMORY;
    default:
      return E_FAIL;
  }
  CBZip2DecompressorReleaser aReleaser(&aBZStream);
  aBZStream.avail_in = 0;
  while (true)
  {
    if (aBZStream.avail_in == 0)
    {
      aBZStream.next_in = (char *)m_InBuffer;
      UINT32 aProcessedSize;
      RETURN_IF_NOT_S_OK(anInStream->Read(m_InBuffer, kBufferSize, &aProcessedSize));
      aBZStream.avail_in = aProcessedSize;
    }

    aBZStream.next_out = (char *)m_OutBuffer;
    aBZStream.avail_out = kBufferSize;
    aResult = aBZStream.Decompress();
    UINT32 aNumBytesToWrite = kBufferSize - aBZStream.avail_out;
    if (aNumBytesToWrite > 0)
    {
      UINT32 aProcessedSize;
      RETURN_IF_NOT_S_OK(anOutStream->Write(m_OutBuffer, aNumBytesToWrite, &aProcessedSize));
      if (aNumBytesToWrite != aProcessedSize)
        return E_FAIL;
    }

    if (aResult == BZ_STREAM_END)
      break;
    switch(aResult)
    {
      case BZ_DATA_ERROR:
      case BZ_DATA_ERROR_MAGIC:
        return S_FALSE;
      case BZ_OK:
        break;
      case BZ_MEM_ERROR:
        return E_OUTOFMEMORY;
      default:
        return E_FAIL;
    }
    if (aProgress != NULL)
    {
      UINT64 aTotalIn = aBZStream.GetTotalIn();
      UINT64 aTotalOut = aBZStream.GetTotalOut();
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aTotalIn, &aTotalOut));
    }
  }
  // aResult = aBZStream.End();

  return S_OK;
}

STDMETHODIMP CCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  try
  {
    return CodeReal(anInStream, anOutStream, anInSize, anOutSize, aProgress);
  }
  /*
  catch(const NStream::NWindow::COutWriteException &anOutWriteException)
  {
    return anOutWriteException.m_Result;
  }
  */
  catch(...)
  {
    return S_FALSE;
  }
}

}}}
