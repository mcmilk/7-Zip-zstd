// BZip2Decoder.cpp

#include "StdAfx.h"

#include "BZip2Decoder.h"

#include "Windows/Defs.h"
#include "Original/bzlib.h"

namespace NCompress {
namespace NBZip2 {

static const UINT32 kBufferSize = (1 << 20);

CDecoder::CDecoder()
{
  m_InBuffer = new BYTE[kBufferSize];
  m_OutBuffer = new BYTE[kBufferSize];
}

CDecoder::~CDecoder()
{
  delete []m_OutBuffer;
  delete []m_InBuffer;
}

struct CBZip2Decompressor: public bz_stream
{
  // bz_stream m_Object;
public:
  int Init(int verbosity, int small) { return BZ2_bzDecompressInit(this, verbosity, small); }
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

STDMETHODIMP CDecoder::CodeReal(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
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
      UINT32 processedSize;
      RINOK(inStream->Read(m_InBuffer, kBufferSize, &processedSize));
      bzStream.avail_in = processedSize;
    }

    bzStream.next_out = (char *)m_OutBuffer;
    bzStream.avail_out = kBufferSize;
    result = bzStream.Decompress();
    UINT32 numBytesToWrite = kBufferSize - bzStream.avail_out;
    if (numBytesToWrite > 0)
    {
      UINT32 processedSize;
      RINOK(outStream->Write(m_OutBuffer, numBytesToWrite, &processedSize));
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
      UINT64 totalIn = bzStream.GetTotalIn();
      UINT64 totalOut = bzStream.GetTotalOut();
      RINOK(progress->SetRatioInfo(&totalIn, &totalOut));
    }
  }
  // result = bzStream.End();

  return S_OK;
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
  catch(...) { return S_FALSE; }
}

}}
