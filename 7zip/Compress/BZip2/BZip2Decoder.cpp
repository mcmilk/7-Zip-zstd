// BZip2Decoder.cpp

#include "StdAfx.h"

#include "BZip2Decoder.h"

#include "../../../Common/Alloc.h"
#include "Original/bzlib.h"

namespace NCompress {
namespace NBZip2 {

static const UInt32 kBufferSize = (1 << 20);

CDecoder::~CDecoder()
{
  BigFree(m_InBuffer);
}

struct CBZip2Decompressor: public bz_stream
{
  int Init(int verbosity, int small) { return BZ2_bzDecompressInit(this, verbosity, small); }
  int Decompress()  { return BZ2_bzDecompress(this); }
  int End()  { return BZ2_bzDecompressEnd(this); }
  UInt64 GetTotalIn() const { return (UInt64(total_in_hi32) << 32) +  total_in_lo32; }
  UInt64 GetTotalOut() const { return (UInt64(total_out_hi32) << 32) +  total_out_lo32; }
};

class CBZip2DecompressorReleaser
{
  CBZip2Decompressor *m_Decompressor;
public:
  CBZip2DecompressorReleaser(CBZip2Decompressor *decompressor): m_Decompressor(decompressor) {}
  void Diable() { m_Decompressor = NULL; }
  ~CBZip2DecompressorReleaser()  { if (m_Decompressor != NULL) m_Decompressor->End(); }
};

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  m_InSize = 0;
  if (m_InBuffer == 0)
  {
    m_InBuffer = (Byte *)BigAlloc(kBufferSize * 2);
    if (m_InBuffer == 0)
      return E_OUTOFMEMORY;
  }
  Byte *outBuffer = m_InBuffer + kBufferSize;

  CBZip2Decompressor bzStream;
  bzStream.bzalloc = NULL;
  bzStream.bzfree = NULL;
  bzStream.opaque = NULL;

  int result = bzStream.Init(0, 0);
  switch(result)
  {
    case BZ_OK:
      break;
    case BZ_MEM_ERROR:
      return E_OUTOFMEMORY;
    default:
      return E_FAIL;
  }
  CBZip2DecompressorReleaser releaser(&bzStream);
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
    result = bzStream.Decompress();
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
    if (progress != NULL)
    {
      UInt64 totalIn = bzStream.GetTotalIn();
      UInt64 totalOut = bzStream.GetTotalOut();
      RINOK(progress->SetRatioInfo(&totalIn, &totalOut));
    }
  }
  m_InSize = bzStream.GetTotalIn();
  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(...) { return S_FALSE; }
}

STDMETHODIMP CDecoder::GetInStreamProcessedSize(UInt64 *value)
{
  if (value == NULL)
    return E_INVALIDARG;
  *value = m_InSize;
  return S_OK;
}

}}
