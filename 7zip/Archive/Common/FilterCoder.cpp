// FilterCoder.cpp

#include "StdAfx.h"

#include "FilterCoder.h"
#include "../../../Common/Alloc.h"
#include "../../../Common/Defs.h"

static const int kBufferSize = 1 << 17;

CFilterCoder::CFilterCoder()
{ 
  _buffer = (Byte *)::BigAlloc(kBufferSize); 
}

CFilterCoder::~CFilterCoder() 
{ 
  BigFree(_buffer); 
}

HRESULT CFilterCoder::WriteWithLimit(ISequentialOutStream *outStream, UInt32 size)
{
  if (_outSizeIsDefined)
  {
    UInt64 remSize = _outSize - _nowPos64;
    if (size > remSize)
      size = (UInt32)remSize;
  }
  UInt32 processedSize = 0;
  RINOK(outStream->Write(_buffer, size, &processedSize));
  if (size != processedSize)
    return E_FAIL;
  _nowPos64 += processedSize;
  return S_OK;
}


STDMETHODIMP CFilterCoder::Code(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress)
{
  Init();
  UInt32 bufferPos = 0;
  if (_outSizeIsDefined = (outSize != 0))
    _outSize = *outSize;

  while(NeedMore())
  {
    UInt32 processedSize;
    UInt32 size = kBufferSize - bufferPos;
    RINOK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    UInt32 endPos = bufferPos + processedSize;

    bufferPos = Filter->Filter(_buffer, endPos);
    if (bufferPos > endPos)
    {
      for (; endPos< bufferPos; endPos++)
        _buffer[endPos] = 0;
      bufferPos = Filter->Filter(_buffer, endPos);
    }

    if (bufferPos == 0)
    {
      if (endPos > 0)
        return WriteWithLimit(outStream, endPos);
      return S_OK;
    }
    RINOK(WriteWithLimit(outStream, bufferPos));
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&_nowPos64, &_nowPos64));
    }
    UInt32 i = 0;
    while(bufferPos < endPos)
      _buffer[i++] = _buffer[bufferPos++];
    bufferPos = i;
  }
  return S_OK;
}

STDMETHODIMP CFilterCoder::SetOutStream(ISequentialOutStream *outStream)
{
  _bufferPos = 0;
  _outStream = outStream;
  Init();
  return S_OK;
}

STDMETHODIMP CFilterCoder::ReleaseOutStream()
{
  _outStream.Release();
  return S_OK;
};


STDMETHODIMP CFilterCoder::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 processedSizeTotal = 0;
  while(size > 0)
  {
    UInt32 sizeMax = kBufferSize - _bufferPos;
    UInt32 sizeTemp = size;
    if (sizeTemp > sizeMax)
      sizeTemp = sizeMax;
    memmove(_buffer + _bufferPos, data, sizeTemp);
    size -= sizeTemp;
    processedSizeTotal += sizeTemp;
    data = (const Byte *)data + sizeTemp;
    UInt32 endPos = _bufferPos + sizeTemp;
    _bufferPos = Filter->Filter(_buffer, endPos);
    if (_bufferPos == 0)
    {
      _bufferPos = endPos;
      break;
    }
    if (_bufferPos > endPos)
    {
      if (size != 0)
        return E_FAIL;
      break;
    }
    RINOK(WriteWithLimit(_outStream, _bufferPos));
    UInt32 i = 0;
    while(_bufferPos < endPos)
      _buffer[i++] = _buffer[_bufferPos++];
    _bufferPos = i;
  }
  if (processedSize != NULL)
    *processedSize = processedSizeTotal;
  return S_OK;
}

STDMETHODIMP CFilterCoder::WritePart(const void *data, UInt32 size, UInt32 *processedSize)
{
  return Write(data, size, processedSize);
}

STDMETHODIMP CFilterCoder::Flush()
{
  if (_bufferPos != 0)
  {
    UInt32 endPos = Filter->Filter(_buffer, _bufferPos);
    if (endPos > _bufferPos)
    {
      for (; _bufferPos < endPos; _bufferPos++)
        _buffer[_bufferPos] = 0;
      if (Filter->Filter(_buffer, endPos) != endPos)
        return E_FAIL;
    }
    UInt32 processedSize;
    RINOK(_outStream->Write(_buffer, _bufferPos, &processedSize));
    if (_bufferPos != processedSize)
      return E_FAIL;
    _bufferPos = 0;
  }
  CMyComPtr<IOutStreamFlush> flush;
  _outStream.QueryInterface(IID_IOutStreamFlush, &flush);
  if (flush)
    return  flush->Flush();
  return S_OK;
}


STDMETHODIMP CFilterCoder::SetInStream(ISequentialInStream *inStream)
{
  _convertedPosBegin = _convertedPosEnd = _bufferPos = 0;
  _inStream = inStream;
  Init();
  return S_OK;
}

STDMETHODIMP CFilterCoder::ReleaseInStream()
{
  _inStream.Release();
  return S_OK;
};

STDMETHODIMP CFilterCoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 processedSizeTotal = 0;
  while(size > 0)
  {
    if (_convertedPosBegin != _convertedPosEnd)
    {
      UInt32 sizeTemp = MyMin(size, _convertedPosEnd - _convertedPosBegin);
      memmove(data, _buffer + _convertedPosBegin, sizeTemp);
      _convertedPosBegin += sizeTemp;
      data = (void *)((Byte *)data + sizeTemp);
      size -= sizeTemp;
      processedSizeTotal += sizeTemp;
      continue;
    }
    int i;
    for (i = 0; _convertedPosEnd + i < _bufferPos; i++)
      _buffer[i] = _buffer[i + _convertedPosEnd];
    _bufferPos = i;
    _convertedPosBegin = _convertedPosEnd = 0;
    UInt32 processedSizeTemp;
    UInt32 size0 = kBufferSize - _bufferPos;
    RINOK(_inStream->Read(_buffer + _bufferPos, size0, &processedSizeTemp));
    _bufferPos = _bufferPos + processedSizeTemp;
    _convertedPosEnd = Filter->Filter(_buffer, _bufferPos);
    if (_convertedPosEnd == 0)
    {
      break;
    }
    if (_convertedPosEnd > _bufferPos)
    {
      for (; _bufferPos < _convertedPosEnd; _bufferPos++)
        _buffer[_bufferPos] = 0;
      _convertedPosEnd = Filter->Filter(_buffer, _bufferPos);
    }
  }
  if (processedSize != NULL)
    *processedSize = processedSizeTotal;
  return S_OK;
}

STDMETHODIMP CFilterCoder::ReadPart(void *data, UInt32 size, UInt32 *processedSize)
{
  return Read(data, size, processedSize);
}

#ifndef _NO_CRYPTO
STDMETHODIMP CFilterCoder::CryptoSetPassword(const Byte *data, UInt32 size)
{
  return _setPassword->CryptoSetPassword(data, size);
}
#endif

#ifndef EXTRACT_ONLY
STDMETHODIMP CFilterCoder::WriteCoderProperties(ISequentialOutStream *outStream)
{
  return _writeCoderProperties->WriteCoderProperties(outStream);
}
#endif

STDMETHODIMP CFilterCoder::SetDecoderProperties(ISequentialInStream *inStream)
{
  return _setDecoderProperties->SetDecoderProperties(inStream);
}
