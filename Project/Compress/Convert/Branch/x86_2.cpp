// x86_2.cpp

#include "StdAfx.h"
#include "x86_2.h"

#include "Windows/Defs.h"

inline UINT32 Swap4(UINT32 value)
{
  return (value << 24) | (value >> 24) | 
    ( (value >> 8) & 0xFF00) | ( (value << 8) & 0xFF0000);
}

inline bool IsJcc(BYTE b0, BYTE b1)
{
  return (b0 == 0x0F && (b1 & 0xF0) == 0x80);
}

#ifndef EXTRACT_ONLY

static bool inline Test86MSByte(BYTE b)
{
  return (b == 0 || b == 0xFF);
}

HRESULT CBCJ2_x86_Encoder::Flush()
{
  RETURN_IF_NOT_S_OK(_mainStream.Flush());
  RETURN_IF_NOT_S_OK(_callStream.Flush());
  RETURN_IF_NOT_S_OK(_jumpStream.Flush());
  _rangeEncoder.FlushData();
  return _rangeEncoder.FlushStream();
}

const UINT32 kDefaultLimit = (1 << 24);

HRESULT CBCJ2_x86_Encoder::CodeReal(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress)
{
  if (numInStreams != 1 || numOutStreams != 4)
    return E_INVALIDARG;

  bool sizeIsDefined = false;
  UINT64 inSize;
  if (inSizes != NULL)
    if (inSizes[0] != NULL)
    {
      inSize = *inSizes[0];
      if (inSize <= kDefaultLimit)
        sizeIsDefined = true;
    }


  ISequentialInStream *inStream = inStreams[0];

  _mainStream.Init(outStreams[0]);
  _callStream.Init(outStreams[1]);
  _jumpStream.Init(outStreams[2]);
  _rangeEncoder.Init(outStreams[3]);
  for (int i = 0; i < 256; i++)
    _statusE8Encoder[i].Init();
  _statusE9Encoder.Init();
  _statusJccEncoder.Init();
  CCoderReleaser releaser(this);

  CComPtr<ICompressGetSubStreamSize> getSubStreamSize;
  {
    inStream->QueryInterface(IID_ICompressGetSubStreamSize, (void **)&getSubStreamSize);
  }


  UINT32 nowPos = 0;
  UINT64 nowPos64 = 0;
  UINT32 bufferPos = 0;
  UINT32 processedSize;

  BYTE prevByte = 0;

  UINT64 subStreamIndex = 0;
  UINT64 subStreamStartPos  = 0;
  UINT64 subStreamEndPos = 0;

  while(true)
  {
    UINT32 size = kBufferSize - bufferPos;
    RETURN_IF_NOT_S_OK(inStream->Read(_buffer + bufferPos, size, &processedSize));
    UINT32 endPos = bufferPos + processedSize;
    
    if (endPos < 5)
    {
      // change it 
      for (bufferPos = 0; bufferPos < endPos; bufferPos++)
      {
        BYTE b = _buffer[bufferPos];
        _mainStream.WriteByte(b);
        if (b == 0xE8)
          _statusE8Encoder[prevByte].Encode(&_rangeEncoder, 0);
        else if (b == 0xE9)
          _statusE9Encoder.Encode(&_rangeEncoder, 0);
        else if (IsJcc(prevByte, b))
          _statusJccEncoder.Encode(&_rangeEncoder, 0);
        prevByte = b;
      }
      return Flush();
    }

    bufferPos = 0;

    UINT32 limit = endPos - 5;
    while(bufferPos <= limit)
    {
      BYTE b = _buffer[bufferPos];
      _mainStream.WriteByte(b);
      if (b != 0xE8 && b != 0xE9 && !IsJcc(prevByte, b))
      {
        bufferPos++;
        prevByte = b;
        continue;
      }
      BYTE nextByte = _buffer[bufferPos + 4];
      UINT32 src = 
        (UINT32(nextByte) << 24) |
        (UINT32(_buffer[bufferPos + 3]) << 16) |
        (UINT32(_buffer[bufferPos + 2]) << 8) |
        (_buffer[bufferPos + 1]);
      UINT32 dest = (nowPos + bufferPos + 5) + src;
      // if (Test86MSByte(nextByte))
      bool convert;
      if (getSubStreamSize != NULL)
      {
        UINT64 currentPos = (nowPos64 + bufferPos);
        while (subStreamEndPos < currentPos)
        {
          UINT64 subStreamSize;
          HRESULT result = getSubStreamSize->GetSubStreamSize(subStreamIndex, &subStreamSize);
          if (result == S_OK)
          {
            subStreamStartPos = subStreamEndPos;
            subStreamEndPos += subStreamSize;          
            subStreamIndex++;
          }
          else if (result == S_FALSE || result == E_NOTIMPL)
          {
            getSubStreamSize.Release();
            subStreamStartPos = 0;
            subStreamEndPos = subStreamStartPos - 1;          
          }
          else
            return result;
        }
        if (getSubStreamSize == NULL)
        {
          if (sizeIsDefined)
            convert = (dest < inSize);
          else
            convert = Test86MSByte(nextByte);
        }
        else if (subStreamEndPos - subStreamStartPos > kDefaultLimit)
          convert = Test86MSByte(nextByte);
        else
        {
          UINT64 dest64 = (currentPos + 5) + INT64(INT32(src));
          convert = (dest64 >= subStreamStartPos && dest64 < subStreamEndPos);
        }
      }
      else if (sizeIsDefined)
        convert = (dest < inSize);
      else
        convert = Test86MSByte(nextByte);
      if (convert)
      {
        if (b == 0xE8)
          _statusE8Encoder[prevByte].Encode(&_rangeEncoder, 1);
        else if (b == 0xE9)
          _statusE9Encoder.Encode(&_rangeEncoder, 1);
        else 
          _statusJccEncoder.Encode(&_rangeEncoder, 1);

        dest = Swap4(dest);

        bufferPos += 5;
        if (b == 0xE8)
          _callStream.WriteBytes(&dest, sizeof(dest));
        else 
          _jumpStream.WriteBytes(&dest, sizeof(dest));
        prevByte = nextByte;
      }
      else
      {
        if (b == 0xE8)
          _statusE8Encoder[prevByte].Encode(&_rangeEncoder, 0);
        else if (b == 0xE9)
          _statusE9Encoder.Encode(&_rangeEncoder, 0);
        else
          _statusJccEncoder.Encode(&_rangeEncoder, 0);
        bufferPos++;
        prevByte = b;
      }
    }
    nowPos += bufferPos;
    nowPos64 += bufferPos;

    if (progress != NULL)
    {
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(&nowPos64, NULL));
    }
 
    
    UINT32 i = 0;
    while(bufferPos < endPos)
      _buffer[i++] = _buffer[bufferPos++];
    bufferPos = i;
  }
}

STDMETHODIMP CBCJ2_x86_Encoder::Code(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStreams, inSizes, numInStreams,
      outStreams, outSizes,numOutStreams,
      progress);
  }
  catch(const NStream::COutByteWriteException &outWriteException)
  {
    return outWriteException.Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}

#endif

HRESULT CBCJ2_x86_Decoder::CodeReal(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress)
{
  if (numInStreams != 4 || numOutStreams != 1)
    return E_INVALIDARG;

  _mainInStream.Init(inStreams[0]);
  _callStream.Init(inStreams[1]);
  _jumpStream.Init(inStreams[2]);
  _rangeDecoder.Init(inStreams[3]);
  for (int i = 0; i < 256; i++)
    _statusE8Decoder[i].Init();
  _statusE9Decoder.Init();
  _statusJccDecoder.Init();

  _outStream.Init(outStreams[0]);

  CCoderReleaser releaser(this);

  BYTE prevByte = 0;
  UINT32 processedBytes = 0;
  while(true)
  {
    if (processedBytes > (1 << 20) && progress != NULL)
    {
      UINT64 nowPos64 = _outStream.GetProcessedSize();
      RETURN_IF_NOT_S_OK(progress->SetRatioInfo(NULL, &nowPos64));
      processedBytes = 0;
    }
    processedBytes++;
    BYTE b;
    if (!_mainInStream.ReadByte(b))
      return Flush();
    _outStream.WriteByte(b);
    if (b != 0xE8 && b != 0xE9 && !IsJcc(prevByte, b))
    {
      prevByte = b;
      continue;
    }
    bool status;
    if (b == 0xE8)
      status = (_statusE8Decoder[prevByte].Decode(&_rangeDecoder) == 1);
    else if (b == 0xE9)
      status = (_statusE9Decoder.Decode(&_rangeDecoder) == 1);
    else
      status = (_statusJccDecoder.Decode(&_rangeDecoder) == 1);
    if (status)
    {
      UINT32 src;
      if (b == 0xE8)
      {
        if (!_callStream.ReadBytes(&src, sizeof(src)))
          return S_FALSE;
      }
      else
      {
        if (!_jumpStream.ReadBytes(&src, sizeof(src)))
          return S_FALSE;
      }
      src = Swap4(src);
      UINT32 dest = src - (UINT32(_outStream.GetProcessedSize()) + 4) ;
      _outStream.WriteBytes(&dest, sizeof(dest));
      prevByte = (dest >> 24);
      processedBytes += 4;
    }
    else
      prevByte = b;
  }
}

STDMETHODIMP CBCJ2_x86_Decoder::Code(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress)
{
  try
  {
    return CodeReal(inStreams, inSizes, numInStreams,
      outStreams, outSizes,numOutStreams,
      progress);
  }
  catch(const NStream::COutByteWriteException &outWriteException)
  {
    return outWriteException.Result;
  }
  catch(...)
  {
    return S_FALSE;
  }
}
