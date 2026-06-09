// KanziDecoder.cpp

#include "StdAfx.h"

#include "KanziDecoder.h"
#include "KanziStreams.h"

#include "../../../C/kanzi/src/io/CompressedInputStream.hpp"

#include "../../Windows/System.h"

#include <istream>

namespace NCompress {
namespace NKANZI {

CDecoder::CDecoder():
  _processedIn(0),
  _processedOut(0),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())
{
  _props.Clear();
}

Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *props, UInt32 size))
{
  return ReadPropsFromBytes(_props, props, size) ? S_OK : E_NOTIMPL;
}

Z7_COM7F_IMF(CDecoder::SetNumberOfThreads(UInt32 numThreads))
{
  if (numThreads < 1)
    numThreads = 1;
  if (numThreads > kKanziMaxThreads)
    numThreads = kKanziMaxThreads;
  _numThreads = numThreads;
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 * /* inSize */,
    const UInt64 *outSize, ICompressProgressInfo *progress))
{
  _processedIn = 0;
  _processedOut = 0;

  CInStreamBuf inBuf(inStream, &_processedIn);
  std::istream input(&inBuf);

  try
  {
    kanzi::CompressedInputStream cis(input, (int)_numThreads);
    char buffer[1 << 15];

    for (;;)
    {
      size_t request = sizeof(buffer);
      if (outSize)
      {
        if (_processedOut >= *outSize)
          break;
        const UInt64 rem = *outSize - _processedOut;
        if (rem < request)
          request = (size_t)rem;
      }

      cis.read(buffer, (std::streamsize)request);
      const std::streamsize read = cis.gcount();
      if (read > 0)
      {
        RINOK(WriteKanziOutput(outStream, buffer, (size_t)read,
            progress, &_processedIn, &_processedOut))
      }
      else if (cis)
        return S_FALSE;

      if (outSize && _processedOut >= *outSize)
        break;
      if (!cis)
        break;
    }

    cis.close();
  }
  catch (const CStreamError &e)
  {
    return e.Result;
  }
  catch (...)
  {
    return E_FAIL;
  }

  return inBuf.GetResult();
}

}}
