// Lzx86Converter.cpp

#include "StdAfx.h"

#include "../../Common/Defs.h"

#include "Lzx86Converter.h"

namespace NCompress {
namespace NLzx {

static const UInt32 kResidue = 6 + 4;

void Cx86ConvertOutStream::MakeTranslation()
{
  if (_pos <= kResidue)
    return;
  UInt32 numBytes = _pos - kResidue;
  Byte *buf = _buf;
  for (UInt32 i = 0; i < numBytes;)
  {
    if (buf[i++] == 0xE8)
    {
      Int32 absValue = 0;
      unsigned j;
      for (j = 0; j < 4; j++)
        absValue += (UInt32)buf[i + j] << (j * 8);
      Int32 pos = (Int32)(_processedSize + i - 1);
      if (absValue >= -pos && absValue < (Int32)_translationSize)
      {
        UInt32 offset = (absValue >= 0) ?
            absValue - pos :
            absValue + _translationSize;
        for (j = 0; j < 4; j++)
        {
          buf[i + j] = (Byte)(offset & 0xFF);
          offset >>= 8;
        }
      }
      i += 4;
    }
  }
}

STDMETHODIMP Cx86ConvertOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (!_translationMode)
    return _stream->Write(data, size, processedSize);
  UInt32 realProcessedSize = 0;
  while (realProcessedSize < size)
  {
    UInt32 writeSize = MyMin(size - realProcessedSize, kUncompressedBlockSize - _pos);
    memcpy(_buf + _pos, (const Byte *)data + realProcessedSize, writeSize);
    _pos += writeSize;
    realProcessedSize += writeSize;
    if (_pos == kUncompressedBlockSize)
    {
      RINOK(Flush());
    }
  }
  if (processedSize)
    *processedSize = realProcessedSize;
  return S_OK;
}

HRESULT Cx86ConvertOutStream::Flush()
{
  if (_pos == 0)
    return S_OK;
  if (_translationMode)
    MakeTranslation();
  UInt32 pos = 0;
  do
  {
    UInt32 processed;
    RINOK(_stream->Write(_buf + pos, _pos - pos, &processed));
    if (processed == 0)
      return E_FAIL;
    pos += processed;
  }
  while (pos < _pos);
  _processedSize += _pos;
  _pos = 0;
  _translationMode = (_translationMode && (_processedSize < ((UInt32)1 << 30)));
  return S_OK;
}

}}
