// Compress/BZip2/Encoder.cpp

#include "StdAfx.h"

#include "Encoder.h"
#include "Alien/Compress/BWT/BZip2/bzlib.h"
#include "Windows/Defs.h"

namespace NCompress {
namespace NBZip2 {
namespace NEncoder {


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

struct CBZip2Compressor: public bz_stream
{
  bz_stream m_Object;
public:
  int Init(int aBlockSize100k, int aVerbosity, int aSmall) 
    { return BZ2_bzCompressInit(this, aBlockSize100k, aVerbosity, aSmall); }
  int Compress(int anAction )  { return BZ2_bzCompress(this, anAction ); }
  int End()  { return BZ2_bzCompressEnd(this); }
  UINT64 GetTotalIn() const { return (UINT64(total_in_hi32) << 32) +  total_in_lo32; }
  UINT64 GetTotalOut() const { return (UINT64(total_out_hi32) << 32) +  total_out_lo32; }
};

class CBZip2CompressorReleaser
{
  CBZip2Compressor *m_Compressor;
public:
  CBZip2CompressorReleaser(CBZip2Compressor *aCompressor): m_Compressor(aCompressor) {}
  void Diable() { m_Compressor = NULL; }
  ~CBZip2CompressorReleaser()  { if (m_Compressor!= NULL) m_Compressor->End(); }
};


STDMETHODIMP CCoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  CBZip2Compressor aBZStream;
  aBZStream.bzalloc = NULL;
  aBZStream.bzfree = NULL;
  aBZStream.opaque = NULL;


  int aResult = aBZStream.Init(9, 0, 0);
  switch(aResult)
  {
    case BZ_OK:
      break;
    case BZ_MEM_ERROR:
      return E_OUTOFMEMORY;
    default:
      return E_FAIL;
  }
  CBZip2CompressorReleaser aReleaser(&aBZStream);
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
    bool anAskFinish = (aBZStream.avail_in == 0);
    aResult = aBZStream.Compress(anAskFinish ? BZ_FINISH : BZ_RUN);
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
      case BZ_RUN_OK:
        if (!anAskFinish)
          break;
        return E_FAIL;
      case BZ_FINISH_OK:
        if (anAskFinish)
          break;
        return E_FAIL;
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


}}}
