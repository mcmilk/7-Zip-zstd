// Compress/CopyCoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "CopyCoder.h"

namespace NCompress {

static const UInt32 kBufferSize = 1 << 17;

CCopyCoder::~CCopyCoder()
{
  ::MidFree(_buffer);
}

STDMETHODIMP CCopyCoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (!_buffer)
  {
    _buffer = (Byte *)::MidAlloc(kBufferSize);
    if (!_buffer)
      return E_OUTOFMEMORY;
  }

  TotalSize = 0;
  for (;;)
  {
    UInt32 size = kBufferSize;
    if (outSize && size > *outSize - TotalSize)
      size = (UInt32)(*outSize - TotalSize);
    RINOK(inStream->Read(_buffer, size, &size));
    if (size == 0)
      break;
    if (outStream)
    {
      RINOK(WriteStream(outStream, _buffer, size));
    }
    TotalSize += size;
    if (progress)
    {
      RINOK(progress->SetRatioInfo(&TotalSize, &TotalSize));
    }
  }
  return S_OK;
}

STDMETHODIMP CCopyCoder::GetInStreamProcessedSize(UInt64 *value)
{
  *value = TotalSize;
  return S_OK;
}

HRESULT CopyStream(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  CMyComPtr<ICompressCoder> copyCoder = new CCopyCoder;
  return copyCoder->Code(inStream, outStream, NULL, NULL, progress);
}

HRESULT CopyStream_ExactSize(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt64 size, ICompressProgressInfo *progress)
{
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
  RINOK(copyCoder->Code(inStream, outStream, NULL, &size, progress));
  return copyCoderSpec->TotalSize == size ? S_OK : E_FAIL;
}

}
