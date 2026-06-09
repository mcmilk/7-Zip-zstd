// KanziEncoder.cpp

#include "StdAfx.h"

#include "KanziEncoder.h"
#include "KanziStreams.h"

#include "../../../C/kanzi/src/io/CompressedOutputStream.hpp"

#include "../../Windows/System.h"

#include <istream>
#include <ostream>

#ifndef Z7_EXTRACT_ONLY
namespace NCompress {
namespace NKANZI {

CEncoder::CEncoder():
  _processedIn(0),
  _processedOut(0),
  _inputSize(0),
  _numThreads(NWindows::NSystem::GetNumberOfProcessors())
{
  _props.Clear();
}

static HRESULT SetBlockSize(const PROPVARIANT &prop, UInt32 &blockSize)
{
  UInt64 v;
  if (prop.vt == VT_UI4)
    v = prop.ulVal;
  else if (prop.vt == VT_UI8)
    v = prop.uhVal.QuadPart;
  else
    return E_INVALIDARG;

  if (v > kKanziMaxBlockSize)
    v = kKanziMaxBlockSize;
  blockSize = (UInt32)v;
  return S_OK;
}

static HRESULT SetChecksum(const PROPVARIANT &prop, Byte &checksumBits)
{
  if (prop.vt != VT_UI4)
    return E_INVALIDARG;

  switch (prop.ulVal)
  {
    case 0: checksumBits = 0; return S_OK;
    case 4:
    case 32: checksumBits = 32; return S_OK;
    case 8:
    case 64: checksumBits = 64; return S_OK;
    default: return E_INVALIDARG;
  }
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  _props.Clear();

  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    switch (propIDs[i])
    {
      case NCoderPropID::kLevel:
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        _props.Level = (Byte)prop.ulVal;
        break;
      case NCoderPropID::kNumThreads:
        if (prop.vt != VT_UI4)
          return E_INVALIDARG;
        RINOK(SetNumberOfThreads(prop.ulVal))
        break;
      case NCoderPropID::kBlockSize:
      case NCoderPropID::kBlockSize2:
        RINOK(SetBlockSize(prop, _props.BlockSize))
        break;
      case NCoderPropID::kCheckSize:
        RINOK(SetChecksum(prop, _props.ChecksumBits))
        break;
      default:
        break;
    }
  }

  NormalizeProps(_props);
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps))
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    if (propIDs[i] == NCoderPropID::kExpectedDataSize)
    {
      const PROPVARIANT &prop = coderProps[i];
      if (prop.vt == VT_UI8)
        _inputSize = prop.uhVal.QuadPart;
    }
  }
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::WriteCoderProperties(ISequentialOutStream *outStream))
{
  Byte props[8];
  WritePropsToBytes(_props, props);
  return WriteStream(outStream, props, sizeof(props));
}

Z7_COM7F_IMF(CEncoder::SetNumberOfThreads(UInt32 numThreads))
{
  if (numThreads < 1)
    numThreads = 1;
  if (numThreads > kKanziMaxThreads)
    numThreads = kKanziMaxThreads;
  _numThreads = numThreads;
  return S_OK;
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, const UInt64 *inSize,
    const UInt64 * /* outSize */, ICompressProgressInfo *progress))
{
  _processedIn = 0;
  _processedOut = 0;

  if (inSize)
    _inputSize = *inSize;

  const char *transform;
  const char *entropy;
  UInt32 defaultBlockSize;
  GetLevelParams(_props.Level, transform, entropy, defaultBlockSize);
  UInt32 blockSize = _props.BlockSize == kKanziDefaultBlockSize ? defaultBlockSize : _props.BlockSize;
  if (blockSize < kKanziMinBlockSize)
    blockSize = kKanziMinBlockSize;
  blockSize = (blockSize + 15) & ~(UInt32)15;

  CInStreamBuf inBuf(inStream, &_processedIn);
  COutStreamBuf outBuf(outStream, progress, &_processedIn, &_processedOut);
  std::istream input(&inBuf);
  std::ostream output(&outBuf);

  try
  {
    kanzi::Context ctx;
    ctx.putInt("jobs", (int)_numThreads);
    ctx.putString("entropy", entropy);
    ctx.putString("transform", transform);
    ctx.putInt("blockSize", (int)blockSize);
    ctx.putInt("checksum", (int)_props.ChecksumBits);
    if (_inputSize != 0 && _inputSize < ((UInt64)1 << 48))
      ctx.putLong("fileSize", (kanzi::int64)_inputSize);

    kanzi::CompressedOutputStream cos(output, ctx, false);
    char buffer[1 << 15];
    for (;;)
    {
      input.read(buffer, sizeof(buffer));
      const std::streamsize read = input.gcount();
      if (read > 0)
        cos.write(buffer, read);
      if (!input)
        break;
    }
    cos.close();
  }
  catch (const CStreamError &e)
  {
    return e.Result;
  }
  catch (...)
  {
    return E_FAIL;
  }

  return outBuf.GetResult();
}

}}
#endif
