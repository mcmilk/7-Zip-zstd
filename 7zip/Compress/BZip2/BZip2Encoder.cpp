// BZip2Encoder.cpp

#include "StdAfx.h"

#include "BZip2Encoder.h"

#include "../../../Common/Alloc.h"
#include "Original/bzlib.h"

namespace NCompress {
namespace NBZip2 {

static const UInt32 kBufferSize = (1 << 20);

CEncoder::~CEncoder()
{
  BigFree(m_InBuffer);
}

struct CBZip2Compressor: public bz_stream
{
  int Init(int blockSize100k, int verbosity, int small) 
    { return BZ2_bzCompressInit(this, blockSize100k, verbosity, small); }
  int Compress(int action )  { return BZ2_bzCompress(this, action ); }
  int End()  { return BZ2_bzCompressEnd(this); }
  UInt64 GetTotalIn() const { return (UInt64(total_in_hi32) << 32) +  total_in_lo32; }
  UInt64 GetTotalOut() const { return (UInt64(total_out_hi32) << 32) +  total_out_lo32; }
};

class CBZip2CompressorReleaser
{
  CBZip2Compressor *m_Compressor;
public:
  CBZip2CompressorReleaser(CBZip2Compressor *compressor): m_Compressor(compressor) {}
  void Disable() { m_Compressor = NULL; }
  ~CBZip2CompressorReleaser()  { if (m_Compressor != NULL) m_Compressor->End(); }
};


STDMETHODIMP CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (m_InBuffer == 0)
  {
    m_InBuffer = (Byte *)BigAlloc(kBufferSize * 2);
    if (m_InBuffer == 0)
      E_OUTOFMEMORY;
  }
  Byte *outBuffer = m_InBuffer + kBufferSize;

  CBZip2Compressor bzStream;
  bzStream.bzalloc = NULL;
  bzStream.bzfree = NULL;
  bzStream.opaque = NULL;

  int result = bzStream.Init(9, 0, 0);
  switch(result)
  {
    case BZ_OK:
      break;
    case BZ_MEM_ERROR:
      return E_OUTOFMEMORY;
    default:
      return E_FAIL;
  }
  CBZip2CompressorReleaser releaser(&bzStream);
  bzStream.avail_in = 0;
  while (true)
  {
    if (bzStream.avail_in == 0)
    {
      bzStream.next_in = (char *)m_InBuffer;
      UInt32 processedSize;
      RINOK(inStream->Read(m_InBuffer, kBufferSize, &processedSize));
      bzStream.avail_in = processedSize;
    }

    bzStream.next_out = (char *)outBuffer;
    bzStream.avail_out = kBufferSize;
    bool askFinish = (bzStream.avail_in == 0);
    result = bzStream.Compress(askFinish ? BZ_FINISH : BZ_RUN);
    UInt32 numBytesToWrite = kBufferSize - bzStream.avail_out;
    if (numBytesToWrite > 0)
    {
      UInt32 processedSize;
      RINOK(outStream->Write(outBuffer, numBytesToWrite, &processedSize));
      if (numBytesToWrite != processedSize)
        return E_FAIL;
    }

    if (result == BZ_STREAM_END)
      break;
    switch(result)
    {
      case BZ_RUN_OK:
        if (!askFinish)
          break;
        return E_FAIL;
      case BZ_FINISH_OK:
        if (askFinish)
          break;
        return E_FAIL;
      case BZ_MEM_ERROR:
        return E_OUTOFMEMORY;
      default:
        return E_FAIL;
    }
    if (progress != NULL)
    {
      UInt64 totalIn = bzStream.GetTotalIn();
      UInt64 totalOut = bzStream.GetTotalOut();
      RINOK(progress->SetRatioInfo(&totalIn, &totalOut));
    }
  }
  // result = bzStream.End();
  return S_OK;
}

}}
