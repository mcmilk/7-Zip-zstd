// Compression/CopyCoder.cpp

#include "StdAfx.h"

#include "Compression/CopyCoder.h"
#include "Common/Defs.h"

#include "Windows/Defs.h"

namespace NCompression {

static const UINT32 kBufferSize = 1 << 17;

CCopyCoder::CCopyCoder()
{
  _buffer = new BYTE[kBufferSize];
}

CCopyCoder::~CCopyCoder()
{
  delete []_buffer;
}

STDMETHODIMP CCopyCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  UINT64 totalSize = 0;
  while(true)
  {
    UINT32 realProcessedSize;
    RETURN_IF_NOT_S_OK(inStream->Read(_buffer, kBufferSize, &realProcessedSize));
    if(realProcessedSize == 0)
      break;
    RETURN_IF_NOT_S_OK(outStream->Write(_buffer, realProcessedSize, NULL));
    totalSize += realProcessedSize;
    if (progress != NULL)
    {
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&totalSize, &totalSize));
    }
  }
  return S_OK;
}

}