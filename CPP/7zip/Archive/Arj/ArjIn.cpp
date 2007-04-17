// Archive/arj/InEngine.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Buffer.h"

#include "../../Common/StreamUtils.h"

#include "ArjIn.h"

extern "C" 
{ 
  #include "../../../../C/7zCrc.h" 
}

namespace NArchive {
namespace NArj {
 
HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize;
  HRESULT result = ReadStream(_stream, data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  IncreasePositionValue(realProcessedSize);
  return result;
}

static inline UInt16 GetUInt16FromMemLE(const Byte *p)
{
  return (UInt16)(p[0] | (((UInt16)p[1]) << 8));
}

static inline UInt32 GetUInt32FromMemLE(const Byte *p)
{
  return p[0] | (((UInt32)p[1]) << 8) | (((UInt32)p[2]) << 16) | (((UInt32)p[3]) << 24);
}

inline bool TestMarkerCandidate(const void *testBytes, UInt32 maxSize)
{
  if (maxSize < 2 + 2 + 4)
    return false;
  const Byte *block = ((const Byte *)(testBytes));
  if (block[0] != NSignature::kSig0 || block[1] != NSignature::kSig1)
    return false;
  UInt32 blockSize = GetUInt16FromMemLE(block + 2);
  if (maxSize < 2 + 2 + blockSize + 4)
    return false;
  block += 4;
  if (blockSize == 0 || blockSize > 2600)
    return false;
  UInt32 crcFromFile = GetUInt32FromMemLE(block + blockSize);
  return (crcFromFile == CrcCalc(block, blockSize));
}

bool CInArchive::FindAndReadMarker(const UInt64 *searchHeaderSizeLimit)
{
  // _archiveInfo.StartPosition = 0;
  _position = _streamStartPosition;
  if(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  const int kMarkerSizeMax = 2 + 2 + kMaxBlockSize + 4;

  CByteBuffer byteBuffer;
  static const UInt32 kSearchMarkerBufferSize = 0x10000;
  byteBuffer.SetCapacity(kSearchMarkerBufferSize);
  Byte *buffer = byteBuffer;

  UInt32 processedSize; 
  ReadBytes(buffer, kMarkerSizeMax, &processedSize);
  if (processedSize == 0)
    return false;
  if (TestMarkerCandidate(buffer, processedSize))
  {
    _position = _streamStartPosition;
    if(_stream->Seek(_position, STREAM_SEEK_SET, NULL) != S_OK)
      return false;
    return true;
  }

  UInt32 numBytesPrev = processedSize - 1;
  memmove(buffer, buffer + 1, numBytesPrev);
  UInt64 curTestPos = _streamStartPosition + 1;
  for (;;)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - _streamStartPosition > *searchHeaderSizeLimit)
        return false;
    UInt32 numReadBytes = kSearchMarkerBufferSize - numBytesPrev;
    ReadBytes(buffer + numBytesPrev, numReadBytes, &processedSize);
    UInt32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < 1)
      return false;
    UInt32 numTests = numBytesInBuffer;
    for(UInt32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestMarkerCandidate(buffer + pos, numBytesInBuffer - pos))
      {
        // _archiveInfo.StartPosition = curTestPos;
        _position = curTestPos;
        if(_stream->Seek(_position, STREAM_SEEK_SET, NULL) != S_OK)
          return false;
        return true;
      }
    }
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numBytesPrev);
  }
}

void CInArchive::IncreasePositionValue(UInt64 addValue)
{
  _position += addValue;
}

void CInArchive::IncreaseRealPosition(UInt64 addValue)
{
  if(_stream->Seek(addValue, STREAM_SEEK_CUR, &_position) != S_OK)
    throw CInArchiveException(CInArchiveException::kSeekStreamError);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  if(ReadBytes(data, size, &realProcessedSize) != S_OK)
    throw CInArchiveException(CInArchiveException::kReadStreamError);
  return (realProcessedSize == size);
}

void CInArchive::SafeReadBytes(void *data, UInt32 size)
{
  if(!ReadBytesAndTestSize(data, size))
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

Byte CInArchive::SafeReadByte()
{
  Byte b;
  SafeReadBytes(&b, 1);
  return b;
}

UInt16 CInArchive::SafeReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = SafeReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::SafeReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = SafeReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

bool CInArchive::ReadBlock()
{
  _blockPos = 0;
  _blockSize = SafeReadUInt16();
  if (_blockSize == 0 || _blockSize > kMaxBlockSize)
    return false;
  SafeReadBytes(_block, _blockSize);
  UInt32 crcFromFile = SafeReadUInt32();
  if (crcFromFile != CrcCalc(_block, _blockSize))
    throw CInArchiveException(CInArchiveException::kCRCError);
  return true;
}

bool CInArchive::ReadBlock2()
{
  Byte id[2];
  ReadBytesAndTestSize(id, 2);
  if (id[0] != NSignature::kSig0 || id[1] != NSignature::kSig1)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  return ReadBlock();
}

bool CInArchive::Open(IInStream *inStream, const UInt64 *searchHeaderSizeLimit)
{
  _stream = inStream;
  if(_stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition) != S_OK)
    return false;
  _position = _streamStartPosition;
  if (!FindAndReadMarker(searchHeaderSizeLimit))
    return false;
  if (!ReadBlock2())
    return false;
  for (;;)
    if (!ReadBlock())
      break;
  return true;
}

void CInArchive::Close()
{
  _stream.Release();
}

void CInArchive::ThrowIncorrectArchiveException()
{
  throw CInArchiveException(CInArchiveException::kIncorrectArchive);
}

Byte CInArchive::ReadByte()
{
  if (_blockPos >= _blockSize)
    ThrowIncorrectArchiveException();
  return _block[_blockPos++];
}

UInt16 CInArchive::ReadUInt16()
{
  UInt16 value = 0;
  for (int i = 0; i < 2; i++)
  {
    Byte b = ReadByte();
    value |= (UInt16(b) << (8 * i));
  }
  return value;
}

UInt32 CInArchive::ReadUInt32()
{
  UInt32 value = 0;
  for (int i = 0; i < 4; i++)
  {
    Byte b = ReadByte();
    value |= (UInt32(b) << (8 * i));
  }
  return value;
}

HRESULT CInArchive::GetNextItem(bool &filled, CItemEx &item)
{
  filled  = false;
  if (!ReadBlock2())
    return S_OK;

  Byte firstHeaderSize = ReadByte();
  item.Version = ReadByte();
  item.ExtractVersion = ReadByte();
  item.HostOS = ReadByte();
  item.Flags = ReadByte();
  item.Method = ReadByte();
  item.FileType = ReadByte();
  ReadByte(); // Reserved
  item.ModifiedTime = ReadUInt32();
  item.PackSize = ReadUInt32();
  item.Size = ReadUInt32();
  item.FileCRC = ReadUInt32();
  ReadUInt16(); // FilespecPositionInFilename
  item.FileAccessMode = ReadUInt16();
  ReadByte(); // FirstChapter
  ReadByte(); // LastChapter

  /*
  UInt32 extraData;
  if ((header.Flags & NFileHeader::NFlags::kExtFile) != 0)
    extraData = GetUInt32FromMemLE(_block + pos);
  */
  _blockPos = firstHeaderSize;

  for (; _blockPos < _blockSize;)
    item.Name += (char)ReadByte();

  for (;;)
    if (!ReadBlock())
      break;

  item.DataPosition = _position;

  filled  = true;
  return S_OK;
}  

}}
