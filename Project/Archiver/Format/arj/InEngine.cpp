// Archive/arj/InEngine.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/Buffer.h"
#include "Common/CRC.h"

#include "Windows/Defs.h"

#include "InEngine.h"

namespace NArchive {
namespace NArj {
 
CInArchiveException::CInArchiveException(CCauseType cause):
  Cause(cause)
{}

HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = _stream->Read(data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  IncreasePositionValue(realProcessedSize);
  return result;
}

inline bool TestMarkerCandidate(const void *testBytes, UINT32 maxSize)
{
  if (maxSize < 2 + 2 + 4)
    return false;
  const BYTE *block = ((const BYTE *)(testBytes));
  if (block[0] != NSignature::kSig0 || block[1] != NSignature::kSig1)
    return false;
  UINT16 blockSize = *((const UINT16 *)(block + 2));
  if (maxSize < 2 + 2 + blockSize + 4)
    return false;
  block += 4;
  if (blockSize == 0 || blockSize > 2600)
    return false;
  UINT32 crcFromFile = *(const UINT32 *)(block + blockSize);
  return (CCRC::VerifyDigest(crcFromFile, block, blockSize));
}

bool CInArchive::FindAndReadMarker(const UINT64 *searchHeaderSizeLimit)
{
  // _archiveInfo.StartPosition = 0;
  _position = _streamStartPosition;
  if(_stream->Seek(_streamStartPosition, STREAM_SEEK_SET, NULL) != S_OK)
    return false;

  const kMarkerSizeMax = 2 + 2 + kMaxBlockSize + sizeof(UINT32);

  CByteBuffer byteBuffer;
  static const UINT32 kSearchMarkerBufferSize = 0x10000;
  byteBuffer.SetCapacity(kSearchMarkerBufferSize);
  BYTE *buffer = byteBuffer;

  UINT32 processedSize; 
  ReadBytes(buffer, 2 + 2 + kMaxBlockSize + sizeof(UINT32), &processedSize);
  if (processedSize == 0)
    return false;
  if (TestMarkerCandidate(buffer, processedSize))
  {
    _position = _streamStartPosition;
    if(_stream->Seek(_position, STREAM_SEEK_SET, NULL) != S_OK)
      return false;
    return true;
  }

  UINT32 numBytesPrev = processedSize - 1;
  memmove(buffer, buffer + 1, numBytesPrev);
  UINT64 curTestPos = _streamStartPosition + 1;
  while(true)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - _streamStartPosition > *searchHeaderSizeLimit)
        return false;
    UINT32 numReadBytes = kSearchMarkerBufferSize - numBytesPrev;
    ReadBytes(buffer + numBytesPrev, numReadBytes, &processedSize);
    UINT32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < 1)
      return false;
    UINT32 numTests = numBytesInBuffer;
    for(UINT32 pos = 0; pos < numTests; pos++, curTestPos++)
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

void CInArchive::IncreasePositionValue(UINT64 addValue)
{
  _position += addValue;
}

void CInArchive::IncreaseRealPosition(UINT64 addValue)
{
  if(_stream->Seek(addValue, STREAM_SEEK_CUR, &_position) != S_OK)
    throw CInArchiveException(CInArchiveException::kSeekStreamError);
}

bool CInArchive::ReadBytesAndTestSize(void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  if(ReadBytes(data, size, &realProcessedSize) != S_OK)
    throw CInArchiveException(CInArchiveException::kReadStreamError);
  return (realProcessedSize == size);
}

void CInArchive::SafeReadBytes(void *data, UINT32 size)
{
  if(!ReadBytesAndTestSize(data, size))
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
}

bool CInArchive::ReadBlock()
{
  SafeReadBytes(&_blockSize, sizeof(_blockSize));
  if (_blockSize == 0)
    return false;
  SafeReadBytes(_block, _blockSize);
  UINT32 crcFromFile;
  ReadBytesAndTestSize(&crcFromFile, sizeof(crcFromFile));
  if (!CCRC::VerifyDigest(crcFromFile, _block, _blockSize))
    throw CInArchiveException(CInArchiveException::kCRCError);
  return true;
}

bool CInArchive::ReadBlock2()
{
  BYTE id[2];
  ReadBytesAndTestSize(id, 2);
  if (id[0] != NSignature::kSig0 || id[1] != NSignature::kSig1)
    throw CInArchiveException(CInArchiveException::kIncorrectArchive);
  return ReadBlock();
}

bool CInArchive::Open(IInStream *inStream, const UINT64 *searchHeaderSizeLimit)
{
  _stream = inStream;
  if(_stream->Seek(0, STREAM_SEEK_CUR, &_streamStartPosition) != S_OK)
    return false;
  _position = _streamStartPosition;
  if (!FindAndReadMarker(searchHeaderSizeLimit))
    return false;
  if (!ReadBlock2())
    return false;
  while(true)
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

HRESULT CInArchive::GetNextItem(bool &filled, CItemInfoEx &item)
{
  filled  = false;
  if (!ReadBlock2())
    return S_OK;

  const NFileHeader::CHeader &header = *(const NFileHeader::CHeader *)_block;

  item.Version = header.Version;
  item.ExtractVersion = header.ExtractVersion;
  item.HostOS = header.HostOS;
  item.Flags = header.Flags;
  item.Method = header.Method;
  item.FileType = header.FileType;
  item.ModifiedTime = header.ModifiedTime;
  item.PackSize = header.PackSize;
  item.Size = header.Size;
  item.FileCRC = header.FileCRC;
  item.FileAccessMode = header.FileAccessMode;

  /*
  UINT32 extraData;
  if ((header.Flags & NFileHeader::NFlags::kExtFile) != 0)
    extraData = *(const UINT32 *)(_block + pos);
  */
  int pos = header.FirstHeaderSize;

  for (; pos < _blockSize; pos++)
  {
    char aByte = _block[pos];
    if (aByte == 0)
      break;
    item.Name += aByte;
  }

  while(true)
    if (!ReadBlock())
      break;

  item.DataPosition = _position;

  filled  = true;
  return S_OK;
}  

}}
