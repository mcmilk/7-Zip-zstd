// Compress/CopyCoder.cpp

#include "StdAfx.h"

#include "CopyCoder.h"

namespace NCompress {

static const UINT32 kBufferSize = 1 << 17;

CCopyCoder::CCopyCoder(): 
  TotalSize(0) 
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
  TotalSize = 0;
  while(true)
  {
    UINT32 realProcessedSize;
    RINOK(inStream->ReadPart(_buffer, kBufferSize, &realProcessedSize));
    if(realProcessedSize == 0)
      break;
    RINOK(outStream->Write(_buffer, realProcessedSize, NULL));
    TotalSize += realProcessedSize;
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&TotalSize, &TotalSize));
    }
  }
  return S_OK;
}

}

