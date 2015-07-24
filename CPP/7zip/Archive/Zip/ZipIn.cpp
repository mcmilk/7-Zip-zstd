// Archive/ZipIn.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../Common/DynamicBuffer.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/StreamUtils.h"

#include "../IArchive.h"

#include "ZipIn.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

namespace NArchive {
namespace NZip {

struct CEcd
{
  UInt16 thisDiskNumber;
  UInt16 startCDDiskNumber;
  UInt16 numEntriesInCDOnThisDisk;
  UInt16 numEntriesInCD;
  UInt32 cdSize;
  UInt32 cdStartOffset;
  UInt16 commentSize;
  
  void Parse(const Byte *p);

  bool IsEmptyArc()
  {
    return thisDiskNumber == 0 && startCDDiskNumber == 0 &&
        numEntriesInCDOnThisDisk == 0 && numEntriesInCD == 0 && cdSize == 0
        && cdStartOffset == 0 // test it
    ;
  }
};

void CEcd::Parse(const Byte *p)
{
  thisDiskNumber = Get16(p);
  startCDDiskNumber = Get16(p + 2);
  numEntriesInCDOnThisDisk = Get16(p + 4);
  numEntriesInCD = Get16(p + 6);
  cdSize = Get32(p + 8);
  cdStartOffset = Get32(p + 12);
  commentSize = Get16(p + 16);
}

struct CEcd64
{
  UInt16 versionMade;
  UInt16 versionNeedExtract;
  UInt32 thisDiskNumber;
  UInt32 startCDDiskNumber;
  UInt64 numEntriesInCDOnThisDisk;
  UInt64 numEntriesInCD;
  UInt64 cdSize;
  UInt64 cdStartOffset;
  
  void Parse(const Byte *p);
  CEcd64() { memset(this, 0, sizeof(*this)); }
};

void CEcd64::Parse(const Byte *p)
{
  versionMade = Get16(p);
  versionNeedExtract = Get16(p + 2);
  thisDiskNumber = Get32(p + 4);
  startCDDiskNumber = Get32(p + 8);
  numEntriesInCDOnThisDisk = Get64(p + 12);
  numEntriesInCD = Get64(p + 20);
  cdSize = Get64(p + 28);
  cdStartOffset = Get64(p + 36);
}

HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit)
{
  _inBufMode = false;
  Close();
  RINOK(stream->Seek(0, STREAM_SEEK_CUR, &m_Position));
  RINOK(stream->Seek(0, STREAM_SEEK_END, &ArcInfo.FileEndPos));
  RINOK(stream->Seek(m_Position, STREAM_SEEK_SET, NULL));

  // printf("\nOpen offset = %d", (int)m_Position);
  RINOK(FindAndReadMarker(stream, searchHeaderSizeLimit));
  RINOK(stream->Seek(m_Position, STREAM_SEEK_SET, NULL));
  Stream = stream;
  return S_OK;
}

void CInArchive::Close()
{
  IsArc = false;
  HeadersError = false;
  HeadersWarning = false;
  ExtraMinorError = false;
  UnexpectedEnd = false;
  NoCentralDir = false;
  IsZip64 = false;
  Stream.Release();
}

HRESULT CInArchive::Seek(UInt64 offset)
{
  return Stream->Seek(offset, STREAM_SEEK_SET, NULL);
}

static bool CheckDosTime(UInt32 dosTime)
{
  if (dosTime == 0)
    return true;
  unsigned month = (dosTime >> 21) & 0xF;
  unsigned day = (dosTime >> 16) & 0x1F;
  unsigned hour = (dosTime >> 11) & 0x1F;
  unsigned min = (dosTime >> 5) & 0x3F;
  unsigned sec = (dosTime & 0x1F) * 2;
  if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  return true;
}

API_FUNC_IsArc IsArc_Zip(const Byte *p, size_t size)
{
  if (size < 8)
    return k_IsArc_Res_NEED_MORE;
  if (p[0] != 'P')
    return k_IsArc_Res_NO;

  UInt32 value = Get32(p);

  if (value == NSignature::kNoSpan)
  {
    p += 4;
    size -= 4;
  }

  value = Get32(p);

  if (value == NSignature::kEcd)
  {
    if (size < kEcdSize)
      return k_IsArc_Res_NEED_MORE;
    CEcd ecd;
    ecd.Parse(p + 4);
    // if (ecd.cdSize != 0)
    if (!ecd.IsEmptyArc())
      return k_IsArc_Res_NO;
    return k_IsArc_Res_YES; // k_IsArc_Res_YES_2;
  }
  
  if (value != NSignature::kLocalFileHeader)
    return k_IsArc_Res_NO;

  if (size < kLocalHeaderSize)
    return k_IsArc_Res_NEED_MORE;
  
  p += 4;

  {
    const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return k_IsArc_Res_NEED_MORE;
  }

  /*
  if (p[0] >= 128) // ExtractVersion.Version;
    return k_IsArc_Res_NO;
  */

  // ExtractVersion.Version = p[0];
  // ExtractVersion.HostOS = p[1];
  // Flags = Get16(p + 2);
  // Method = Get16(p + 4);
  /*
  // 9.33: some zip archives contain incorrect value in timestamp. So we don't check it now
  UInt32 dosTime = Get32(p + 6);
  if (!CheckDosTime(dosTime))
    return k_IsArc_Res_NO;
  */
  // Crc = Get32(p + 10);
  // PackSize = Get32(p + 14);
  // Size = Get32(p + 18);
  unsigned nameSize = Get16(p + 22);
  unsigned extraSize = Get16(p + 24);
  UInt32 extraOffset = kLocalHeaderSize + (UInt32)nameSize;
  if (extraOffset + extraSize > (1 << 16))
    return k_IsArc_Res_NO;

  p -= 4;

  {
    size_t rem = size - kLocalHeaderSize;
    if (rem > nameSize)
      rem = nameSize;
    const Byte *p2 = p + kLocalHeaderSize;
    for (size_t i = 0; i < rem; i++)
      if (p2[i] == 0)
        return k_IsArc_Res_NO;
  }

  if (size < extraOffset)
    return k_IsArc_Res_NEED_MORE;

  if (extraSize > 0)
  {
    p += extraOffset;
    size -= extraOffset;
    while (extraSize != 0)
    {
      if (extraSize < 4)
      {
        // 7-Zip before 9.31 created incorrect WsAES Extra in folder's local headers.
        // so we return k_IsArc_Res_YES to support such archives.
        // return k_IsArc_Res_NO; // do we need to support such extra ?
        return k_IsArc_Res_YES;
      }
      if (size < 4)
        return k_IsArc_Res_NEED_MORE;
      unsigned dataSize = Get16(p + 2);
      size -= 4;
      extraSize -= 4;
      p += 4;
      if (dataSize > extraSize)
        return k_IsArc_Res_NO;
      if (dataSize > size)
        return k_IsArc_Res_NEED_MORE;
      size -= dataSize;
      extraSize -= dataSize;
      p += dataSize;
    }
  }
  
  return k_IsArc_Res_YES;
}

static UInt32 IsArc_Zip_2(const Byte *p, size_t size, bool isFinal)
{
  UInt32 res = IsArc_Zip(p, size);
  if (res == k_IsArc_Res_NEED_MORE && isFinal)
    return k_IsArc_Res_NO;
  return res;
}

HRESULT CInArchive::FindAndReadMarker(IInStream *stream, const UInt64 *searchLimit)
{
  ArcInfo.Clear();
  ArcInfo.MarkerPos = m_Position;
  ArcInfo.MarkerPos2 = m_Position;

  if (searchLimit && *searchLimit == 0)
  {
    const unsigned kStartBufSize = kMarkerSize;
    Byte startBuf[kStartBufSize];
    size_t processed = kStartBufSize;
    RINOK(ReadStream(stream, startBuf, &processed));
    m_Position += processed;
    if (processed < kMarkerSize)
      return S_FALSE;
    m_Signature = Get32(startBuf);
    if (m_Signature != NSignature::kEcd &&
        m_Signature != NSignature::kLocalFileHeader)
    {
      if (m_Signature != NSignature::kNoSpan)
        return S_FALSE;
      size_t processed = kStartBufSize;
      RINOK(ReadStream(stream, startBuf, &processed));
      m_Position += processed;
      if (processed < kMarkerSize)
        return S_FALSE;
      m_Signature = Get32(startBuf);
      if (m_Signature != NSignature::kEcd &&
          m_Signature != NSignature::kLocalFileHeader)
        return S_FALSE;
      ArcInfo.MarkerPos2 += 4;
    }

    // we use weak test in case of *searchLimit == 0)
    // since error will be detected later in Open function
    // m_Position = ArcInfo.MarkerPos2 + 4;
    return S_OK; // maybe we need to search backward.
  }

  const size_t kBufSize = (size_t)1 << 18; // must be larger than kCheckSize
  const size_t kCheckSize = (size_t)1 << 16; // must be smaller than kBufSize
  CByteArr buffer(kBufSize);
  
  size_t numBytesInBuffer = 0;
  UInt64 curScanPos = 0;

  for (;;)
  {
    size_t numReadBytes = kBufSize - numBytesInBuffer;
    RINOK(ReadStream(stream, buffer + numBytesInBuffer, &numReadBytes));
    m_Position += numReadBytes;
    numBytesInBuffer += numReadBytes;
    bool isFinished = (numBytesInBuffer != kBufSize);
    
    size_t limit = (isFinished ? numBytesInBuffer : numBytesInBuffer - kCheckSize);

    if (searchLimit && curScanPos + limit > *searchLimit)
      limit = (size_t)(*searchLimit - curScanPos + 1);

    if (limit < 1)
      break;

    const Byte *buf = buffer;
    for (size_t pos = 0; pos < limit; pos++)
    {
      if (buf[pos] != 0x50)
        continue;
      if (buf[pos + 1] != 0x4B)
        continue;
      size_t rem = numBytesInBuffer - pos;
      UInt32 res = IsArc_Zip_2(buf + pos, rem, isFinished);
      if (res != k_IsArc_Res_NO)
      {
        if (rem < kMarkerSize)
          return S_FALSE;
        m_Signature = Get32(buf + pos);
        ArcInfo.MarkerPos += curScanPos + pos;
        ArcInfo.MarkerPos2 = ArcInfo.MarkerPos;
        if (m_Signature == NSignature::kNoSpan)
        {
          m_Signature = Get32(buf + pos + 4);
          ArcInfo.MarkerPos2 += 4;
        }
        m_Position = ArcInfo.MarkerPos2 + kMarkerSize;
        return S_OK;
      }
    }

    if (isFinished)
      break;

    curScanPos += limit;
    numBytesInBuffer -= limit;
    memmove(buffer, buffer + limit, numBytesInBuffer);
  }
  
  return S_FALSE;
}

HRESULT CInArchive::IncreaseRealPosition(Int64 addValue)
{
  return Stream->Seek(addValue, STREAM_SEEK_CUR, &m_Position);
}

class CUnexpectEnd {};

HRESULT CInArchive::ReadBytes(void *data, UInt32 size, UInt32 *processedSize)
{
  size_t realProcessedSize = size;
  HRESULT result = S_OK;
  if (_inBufMode)
  {
    try { realProcessedSize = _inBuffer.ReadBytes((Byte *)data, size); }
    catch (const CInBufferException &e) { return e.ErrorCode; }
  }
  else
    result = ReadStream(Stream, data, &realProcessedSize);
  if (processedSize)
    *processedSize = (UInt32)realProcessedSize;
  m_Position += realProcessedSize;
  return result;
}

void CInArchive::SafeReadBytes(void *data, unsigned size)
{
  size_t processed = size;
  if (_inBufMode)
  {
    processed = _inBuffer.ReadBytes((Byte *)data, size);
    m_Position += processed;
  }
  else
  {
    HRESULT result = ReadStream(Stream, data, &processed);
    m_Position += processed;
    if (result != S_OK)
      throw CSystemException(result);
  }
  if (processed != size)
    throw CUnexpectEnd();
}

void CInArchive::ReadBuffer(CByteBuffer &buffer, unsigned size)
{
  buffer.Alloc(size);
  if (size > 0)
    SafeReadBytes(buffer, size);
}

Byte CInArchive::ReadByte()
{
  Byte b;
  SafeReadBytes(&b, 1);
  return b;
}

UInt16 CInArchive::ReadUInt16() { Byte buf[2]; SafeReadBytes(buf, 2); return Get16(buf); }
UInt32 CInArchive::ReadUInt32() { Byte buf[4]; SafeReadBytes(buf, 4); return Get32(buf); }
UInt64 CInArchive::ReadUInt64() { Byte buf[8]; SafeReadBytes(buf, 8); return Get64(buf); }

void CInArchive::Skip(unsigned num)
{
  if (_inBufMode)
  {
    size_t skip = _inBuffer.Skip(num);
    m_Position += skip;
    if (skip != num)
      throw CUnexpectEnd();
  }
  else
  {
    for (unsigned i = 0; i < num; i++)
      ReadByte();
  }
}

void CInArchive::Skip64(UInt64 num)
{
  for (UInt64 i = 0; i < num; i++)
    ReadByte();
}


void CInArchive::ReadFileName(unsigned size, AString &s)
{
  if (size == 0)
  {
    s.Empty();
    return;
  }
  char *p = s.GetBuf(size);
  SafeReadBytes(p, size);
  s.ReleaseBuf_CalcLen(size);
}

bool CInArchive::ReadExtra(unsigned extraSize, CExtraBlock &extraBlock,
    UInt64 &unpackSize, UInt64 &packSize, UInt64 &localHeaderOffset, UInt32 &diskStartNumber)
{
  extraBlock.Clear();
  UInt32 remain = extraSize;
  while (remain >= 4)
  {
    CExtraSubBlock subBlock;
    subBlock.ID = ReadUInt16();
    unsigned dataSize = ReadUInt16();
    remain -= 4;
    if (dataSize > remain) // it's bug
    {
      HeadersWarning = true;
      Skip(remain);
      return false;
    }
    if (subBlock.ID == NFileHeader::NExtraID::kZip64)
    {
      if (unpackSize == 0xFFFFFFFF)
      {
        if (dataSize < 8)
        {
          HeadersWarning = true;
          Skip(remain);
          return false;
        }
        unpackSize = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (packSize == 0xFFFFFFFF)
      {
        if (dataSize < 8)
          break;
        packSize = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (localHeaderOffset == 0xFFFFFFFF)
      {
        if (dataSize < 8)
          break;
        localHeaderOffset = ReadUInt64();
        remain -= 8;
        dataSize -= 8;
      }
      if (diskStartNumber == 0xFFFF)
      {
        if (dataSize < 4)
          break;
        diskStartNumber = ReadUInt32();
        remain -= 4;
        dataSize -= 4;
      }
      Skip(dataSize);
    }
    else
    {
      ReadBuffer(subBlock.Data, dataSize);
      extraBlock.SubBlocks.Add(subBlock);
    }
    remain -= dataSize;
  }
  if (remain != 0)
  {
    ExtraMinorError = true;
    // 7-Zip before 9.31 created incorrect WsAES Extra in folder's local headers.
    // so we don't return false, but just set warning flag
    // return false;
  }
  Skip(remain);
  return true;
}

bool CInArchive::ReadLocalItem(CItemEx &item)
{
  const unsigned kPureHeaderSize = kLocalHeaderSize - 4;
  Byte p[kPureHeaderSize];
  SafeReadBytes(p, kPureHeaderSize);
  {
    unsigned i;
    for (i = 0; i < kPureHeaderSize && p[i] == 0; i++);
    if (i == kPureHeaderSize)
      return false;
  }

  item.ExtractVersion.Version = p[0];
  item.ExtractVersion.HostOS = p[1];
  item.Flags = Get16(p + 2);
  item.Method = Get16(p + 4);
  item.Time = Get32(p + 6);
  item.Crc = Get32(p + 10);
  item.PackSize = Get32(p + 14);
  item.Size = Get32(p + 18);
  unsigned nameSize = Get16(p + 22);
  unsigned extraSize = Get16(p + 24);
  ReadFileName(nameSize, item.Name);
  item.LocalFullHeaderSize = kLocalHeaderSize + (UInt32)nameSize + extraSize;

  /*
  if (item.IsDir())
    item.Size = 0; // check It
  */

  if (extraSize > 0)
  {
    UInt64 localHeaderOffset = 0;
    UInt32 diskStartNumber = 0;
    if (!ReadExtra(extraSize, item.LocalExtra, item.Size, item.PackSize,
        localHeaderOffset, diskStartNumber))
    {
      /* Most of archives are OK for Extra. But there are some rare cases
         that have error. And if error in first item, it can't open archive.
         So we ignore that error */
      // return false;
    }
  }
  if (!CheckDosTime(item.Time))
  {
    HeadersWarning = true;
    // return false;
  }
  if (item.Name.Len() != nameSize)
    return false;
  return item.LocalFullHeaderSize <= ((UInt32)1 << 16);
}

static bool FlagsAreSame(const CItem &i1, const CItem &i2)
{
  if (i1.Method != i2.Method)
    return false;
  if (i1.Flags == i2.Flags)
    return true;
  UInt32 mask = 0xFFFF;
  switch(i1.Method)
  {
    case NFileHeader::NCompressionMethod::kDeflated:
      mask = 0x7FF9;
      break;
    default:
      if (i1.Method <= NFileHeader::NCompressionMethod::kImploded)
        mask = 0x7FFF;
  }
  return ((i1.Flags & mask) == (i2.Flags & mask));
}

static bool AreItemsEqual(const CItemEx &localItem, const CItemEx &cdItem)
{
  if (!FlagsAreSame(cdItem, localItem))
    return false;
  if (!localItem.HasDescriptor())
  {
    if (cdItem.Crc != localItem.Crc ||
        cdItem.PackSize != localItem.PackSize ||
        cdItem.Size != localItem.Size)
      return false;
  }
  /* pkzip 2.50 creates incorrect archives. It uses
       - WIN encoding for name in local header
       - OEM encoding for name in central header
     We don't support these strange items. */

  /* if (cdItem.Name.Len() != localItem.Name.Len())
    return false;
  */
  if (cdItem.Name != localItem.Name)
    return false;
  return true;
}

HRESULT CInArchive::ReadLocalItemAfterCdItem(CItemEx &item)
{
  if (item.FromLocal)
    return S_OK;
  try
  {
    UInt64 offset = ArcInfo.Base + item.LocalHeaderPos;
    if (ArcInfo.Base < 0 && (Int64)offset < 0)
      return S_FALSE;
    RINOK(Seek(offset));
    CItemEx localItem;
    if (ReadUInt32() != NSignature::kLocalFileHeader)
      return S_FALSE;
    ReadLocalItem(localItem);
    if (!AreItemsEqual(localItem, item))
      return S_FALSE;
    item.LocalFullHeaderSize = localItem.LocalFullHeaderSize;
    item.LocalExtra = localItem.LocalExtra;
    item.FromLocal = true;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}

HRESULT CInArchive::ReadLocalItemDescriptor(CItemEx &item)
{
  const unsigned kBufSize = (1 << 12);
  Byte buf[kBufSize];
  
  UInt32 numBytesInBuffer = 0;
  UInt32 packedSize = 0;
  
  for (;;)
  {
    UInt32 processedSize;
    RINOK(ReadBytes(buf + numBytesInBuffer, kBufSize - numBytesInBuffer, &processedSize));
    numBytesInBuffer += processedSize;
    if (numBytesInBuffer < kDataDescriptorSize)
      return S_FALSE;
    
    UInt32 i;
    for (i = 0; i <= numBytesInBuffer - kDataDescriptorSize; i++)
    {
      // descriptor signature field is Info-ZIP's extension to pkware Zip specification.
      // New ZIP specification also allows descriptorSignature.
      if (buf[i] != 0x50)
        continue;
      // !!!! It must be fixed for Zip64 archives
      if (Get32(buf + i) == NSignature::kDataDescriptor)
      {
        UInt32 descriptorPackSize = Get32(buf + i + 8);
        if (descriptorPackSize == packedSize + i)
        {
          item.Crc = Get32(buf + i + 4);
          item.PackSize = descriptorPackSize;
          item.Size = Get32(buf + i + 12);
          return IncreaseRealPosition((Int64)(Int32)(0 - (numBytesInBuffer - i - kDataDescriptorSize)));
        }
      }
    }
    
    packedSize += i;
    unsigned j;
    for (j = 0; i < numBytesInBuffer; i++, j++)
      buf[j] = buf[i];
    numBytesInBuffer = j;
  }
}

HRESULT CInArchive::ReadLocalItemAfterCdItemFull(CItemEx &item)
{
  if (item.FromLocal)
    return S_OK;
  try
  {
    RINOK(ReadLocalItemAfterCdItem(item));
    if (item.HasDescriptor())
    {
      RINOK(Seek(ArcInfo.Base + item.GetDataPosition() + item.PackSize));
      if (ReadUInt32() != NSignature::kDataDescriptor)
        return S_FALSE;
      UInt32 crc = ReadUInt32();
      UInt64 packSize, unpackSize;

      /*
      if (IsZip64)
      {
        packSize = ReadUInt64();
        unpackSize = ReadUInt64();
      }
      else
      */
      {
        packSize = ReadUInt32();
        unpackSize = ReadUInt32();
      }

      if (crc != item.Crc || item.PackSize != packSize || item.Size != unpackSize)
        return S_FALSE;
    }
  }
  catch(...) { return S_FALSE; }
  return S_OK;
}
  
HRESULT CInArchive::ReadCdItem(CItemEx &item)
{
  item.FromCentral = true;
  Byte p[kCentralHeaderSize - 4];
  SafeReadBytes(p, kCentralHeaderSize - 4);

  item.MadeByVersion.Version = p[0];
  item.MadeByVersion.HostOS = p[1];
  item.ExtractVersion.Version = p[2];
  item.ExtractVersion.HostOS = p[3];
  item.Flags = Get16(p + 4);
  item.Method = Get16(p + 6);
  item.Time = Get32(p + 8);
  item.Crc = Get32(p + 12);
  item.PackSize = Get32(p + 16);
  item.Size = Get32(p + 20);
  unsigned nameSize = Get16(p + 24);
  UInt16 extraSize = Get16(p + 26);
  UInt16 commentSize = Get16(p + 28);
  UInt32 diskNumberStart = Get16(p + 30);
  item.InternalAttrib = Get16(p + 32);
  item.ExternalAttrib = Get32(p + 34);
  item.LocalHeaderPos = Get32(p + 38);
  ReadFileName(nameSize, item.Name);
  
  if (extraSize > 0)
  {
    ReadExtra(extraSize, item.CentralExtra, item.Size, item.PackSize,
        item.LocalHeaderPos, diskNumberStart);
  }

  if (diskNumberStart != 0)
    return E_NOTIMPL;
  
  // May be these strings must be deleted
  /*
  if (item.IsDir())
    item.Size = 0;
  */
  
  ReadBuffer(item.Comment, commentSize);
  return S_OK;
}

void CCdInfo::ParseEcd(const Byte *p)
{
  NumEntries = Get16(p + 10);
  Size = Get32(p + 12);
  Offset = Get32(p + 16);
}

void CCdInfo::ParseEcd64(const Byte *p)
{
  NumEntries = Get64(p + 24);
  Size = Get64(p + 40);
  Offset = Get64(p + 48);
}

HRESULT CInArchive::TryEcd64(UInt64 offset, CCdInfo &cdInfo)
{
  if (offset >= ((UInt64)1 << 63))
    return S_FALSE;
  RINOK(Seek(offset));
  Byte buf[kEcd64_FullSize];

  RINOK(ReadStream_FALSE(Stream, buf, kEcd64_FullSize));

  if (Get32(buf) != NSignature::kEcd64)
    return S_FALSE;
  UInt64 mainSize = Get64(buf + 4);
  if (mainSize < kEcd64_MainSize || mainSize > ((UInt64)1 << 32))
    return S_FALSE;
  cdInfo.ParseEcd64(buf);
  return S_OK;
}

HRESULT CInArchive::FindCd(CCdInfo &cdInfo)
{
  UInt64 endPosition;
  RINOK(Stream->Seek(0, STREAM_SEEK_END, &endPosition));
  
  const UInt32 kBufSizeMax = ((UInt32)1 << 16) + kEcdSize + kEcd64Locator_Size + kEcd64_FullSize;
  UInt32 bufSize = (endPosition < kBufSizeMax) ? (UInt32)endPosition : kBufSizeMax;
  if (bufSize < kEcdSize)
    return S_FALSE;
  CByteArr byteBuffer(bufSize);

  UInt64 startPosition = endPosition - bufSize;
  RINOK(Stream->Seek(startPosition, STREAM_SEEK_SET, &m_Position));
  if (m_Position != startPosition)
    return S_FALSE;
  
  RINOK(ReadStream_FALSE(Stream, byteBuffer, bufSize));
  
  const Byte *buf = byteBuffer;
  for (UInt32 i = bufSize - kEcdSize;; i--)
  {
    if (buf[i] != 0x50)
    {
      if (i == 0) return S_FALSE;
      i--;
      if (buf[i] != 0x50)
      {
        if (i == 0) return S_FALSE;
        continue;
      }
    }
    if (Get32(buf + i) == NSignature::kEcd)
    {
      if (i >= kEcd64_FullSize + kEcd64Locator_Size)
      {
        const Byte *locator = buf + i - kEcd64Locator_Size;
        if (Get32(locator) == NSignature::kEcd64Locator &&
            Get32(locator + 4) == 0) // number of the disk with the start of the zip64 ECD
        {
          // Most of the zip64 use fixed size Zip64 ECD
          
          UInt64 ecd64Offset = Get64(locator + 8);
          UInt64 absEcd64 = endPosition - bufSize + i - (kEcd64Locator_Size + kEcd64_FullSize);
          {
            const Byte *ecd64 = locator - kEcd64_FullSize;
            if (Get32(ecd64) == NSignature::kEcd64 &&
                Get64(ecd64 + 4) == kEcd64_MainSize)
            {
              cdInfo.ParseEcd64(ecd64);
              ArcInfo.Base = absEcd64 - ecd64Offset;
              return S_OK;
            }
          }

          // some zip64 use variable size Zip64 ECD.
          // we try to find it
          if (absEcd64 != ecd64Offset)
          {
            if (TryEcd64(ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = 0;
              return S_OK;
            }
          }
          if (ArcInfo.MarkerPos != 0 &&
              ArcInfo.MarkerPos + ecd64Offset != absEcd64)
          {
            if (TryEcd64(ArcInfo.MarkerPos + ecd64Offset, cdInfo) == S_OK)
            {
              ArcInfo.Base = ArcInfo.MarkerPos;
              return S_OK;
            }
          }
        }
      }
      if (Get32(buf + i + 4) == 0) // ThisDiskNumber, StartCentralDirectoryDiskNumber;
      {
        cdInfo.ParseEcd(buf + i);
        UInt64 absEcdPos = endPosition - bufSize + i;
        UInt64 cdEnd = cdInfo.Size + cdInfo.Offset;
        ArcInfo.Base = 0;
        if (absEcdPos != cdEnd)
        {
          /*
          if (cdInfo.Offset <= 16 && cdInfo.Size != 0)
          {
            // here we support some rare ZIP files with Central directory at the start
            ArcInfo.Base = 0;
          }
          else
          */
          ArcInfo.Base = absEcdPos - cdEnd;
        }
        return S_OK;
      }
    }
    if (i == 0)
      return S_FALSE;
  }
}


HRESULT CInArchive::TryReadCd(CObjectVector<CItemEx> &items, UInt64 cdOffset, UInt64 cdSize, CProgressVirt *progress)
{
  items.Clear();
  RINOK(Stream->Seek(cdOffset, STREAM_SEEK_SET, &m_Position));
  if (m_Position != cdOffset)
    return S_FALSE;

  _inBuffer.Init();
  _inBufMode = true;

  while (m_Position - cdOffset < cdSize)
  {
    if (ReadUInt32() != NSignature::kCentralFileHeader)
      return S_FALSE;
    CItemEx cdItem;
    RINOK(ReadCdItem(cdItem));
    items.Add(cdItem);
    if (progress && items.Size() % 1 == 0)
      RINOK(progress->SetCompletedCD(items.Size()));
  }
  return (m_Position - cdOffset == cdSize) ? S_OK : S_FALSE;
}

HRESULT CInArchive::ReadCd(CObjectVector<CItemEx> &items, UInt64 &cdOffset, UInt64 &cdSize, CProgressVirt *progress)
{
  CCdInfo cdInfo;
  RINOK(FindCd(cdInfo));
  HRESULT res = S_FALSE;
  cdSize = cdInfo.Size;
  cdOffset = cdInfo.Offset;
  if (progress)
    progress->SetTotalCD(cdInfo.NumEntries);
  res = TryReadCd(items, ArcInfo.Base + cdOffset, cdSize, progress);
  if (res == S_FALSE && ArcInfo.Base == 0)
  {
    res = TryReadCd(items, ArcInfo.MarkerPos + cdOffset, cdSize, progress);
    if (res == S_OK)
      ArcInfo.Base = ArcInfo.MarkerPos;
  }
  return res;
}

static HRESULT FindItem(const CObjectVector<CItemEx> &items, UInt64 offset)
{
  unsigned left = 0, right = items.Size();
  for (;;)
  {
    if (left >= right)
      return -1;
    unsigned index = (left + right) / 2;
    UInt64 position = items[index].LocalHeaderPos;
    if (offset == position)
      return index;
    if (offset < position)
      right = index;
    else
      left = index + 1;
  }
}

bool IsStrangeItem(const CItem &item)
{
  return item.Name.Len() > (1 << 14) || item.Method > (1 << 8);
}

HRESULT CInArchive::ReadLocals(
    CObjectVector<CItemEx> &items, CProgressVirt *progress)
{
  items.Clear();
  while (m_Signature == NSignature::kLocalFileHeader)
  {
    CItemEx item;
    item.LocalHeaderPos = m_Position - 4 - ArcInfo.MarkerPos;
    // we write ralative LocalHeaderPos here. Later we can correct it to real Base.
    try
    {
      ReadLocalItem(item);
      item.FromLocal = true;
      if (item.HasDescriptor())
        ReadLocalItemDescriptor(item);
      else
      {
        RINOK(IncreaseRealPosition(item.PackSize));
      }
      items.Add(item);
      m_Signature = ReadUInt32();
    }
    catch (CUnexpectEnd &)
    {
      if (items.IsEmpty() || items.Size() == 1 && IsStrangeItem(items[0]))
        return S_FALSE;
      throw;
    }
    if (progress && items.Size() % 1 == 0)
      RINOK(progress->SetCompletedLocal(items.Size(), item.LocalHeaderPos));
  }

  if (items.Size() == 1 && m_Signature != NSignature::kCentralFileHeader)
    if (IsStrangeItem(items[0]))
      return S_FALSE;
  return S_OK;
}


#define COPY_ECD_ITEM_16(n) if (!isZip64 || ecd. n != 0xFFFF)     ecd64. n = ecd. n;
#define COPY_ECD_ITEM_32(n) if (!isZip64 || ecd. n != 0xFFFFFFFF) ecd64. n = ecd. n;

HRESULT CInArchive::ReadHeaders2(CObjectVector<CItemEx> &items, CProgressVirt *progress)
{
  items.Clear();

  // m_Signature must be kLocalFileHeader or kEcd
  // m_Position points to next byte after signature
  RINOK(Stream->Seek(m_Position, STREAM_SEEK_SET, NULL));

  if (!_inBuffer.Create(1 << 15))
    return E_OUTOFMEMORY;
  _inBuffer.SetStream(Stream);

  bool needReadCd = true;
  bool localsWereRead = false;
  if (m_Signature == NSignature::kEcd)
  {
    // It must be empty archive or backware archive
    // we don't support backware archive still
    
    const unsigned kBufSize = kEcdSize - 4;
    Byte buf[kBufSize];
    SafeReadBytes(buf, kBufSize);
    CEcd ecd;
    ecd.Parse(buf);
    // if (ecd.cdSize != 0)
    // Do we need also to support the case where empty zip archive with PK00 uses cdOffset = 4 ??
    if (!ecd.IsEmptyArc())
      return S_FALSE;

    ArcInfo.Base = ArcInfo.MarkerPos;
    needReadCd = false;
    IsArc = true; // check it: we need more tests?
    RINOK(Stream->Seek(ArcInfo.MarkerPos2 + 4, STREAM_SEEK_SET, &m_Position));
  }

  UInt64 cdSize = 0, cdRelatOffset = 0, cdAbsOffset = 0;
  HRESULT res = S_OK;
  
  if (needReadCd)
  {
    CItemEx firstItem;
    // try
    {
      try
      {
        if (!ReadLocalItem(firstItem))
          return S_FALSE;
      }
      catch(CUnexpectEnd &)
      {
        return S_FALSE;
      }

      IsArc = true;
      res = ReadCd(items, cdRelatOffset, cdSize, progress);
      if (res == S_OK)
        m_Signature = ReadUInt32();
    }
    // catch() { res = S_FALSE; }
    if (res != S_FALSE && res != S_OK)
      return res;

    if (res == S_OK && items.Size() == 0)
      res = S_FALSE;

    if (res == S_OK)
    {
      // we can't read local items here to keep _inBufMode state
      firstItem.LocalHeaderPos = ArcInfo.MarkerPos2 - ArcInfo.Base;
      int index = FindItem(items, firstItem.LocalHeaderPos);
      if (index == -1)
        res = S_FALSE;
      else if (!AreItemsEqual(firstItem, items[index]))
        res = S_FALSE;
      ArcInfo.CdWasRead = true;
      ArcInfo.FirstItemRelatOffset = items[0].LocalHeaderPos;
    }
  }

  CObjectVector<CItemEx> cdItems;

  bool needSetBase = false;
  unsigned numCdItems = items.Size();
  
  if (res == S_FALSE)
  {
    // CD doesn't match firstItem so we clear items and read Locals.
    items.Clear();
    localsWereRead = true;
    _inBufMode = false;
    ArcInfo.Base = ArcInfo.MarkerPos;
    RINOK(Stream->Seek(ArcInfo.MarkerPos2, STREAM_SEEK_SET, &m_Position));
    m_Signature = ReadUInt32();

    RINOK(ReadLocals(items, progress));
    
    if (m_Signature != NSignature::kCentralFileHeader)
    {
      m_Position -= 4;
      NoCentralDir = true;
      HeadersError = true;
      return S_OK;
    }
    _inBufMode = true;
    _inBuffer.Init();
    cdAbsOffset = m_Position - 4;
    for (;;)
    {
      CItemEx cdItem;
      RINOK(ReadCdItem(cdItem));
      cdItems.Add(cdItem);
      if (progress && cdItems.Size() % 1 == 0)
        RINOK(progress->SetCompletedCD(items.Size()));
      m_Signature = ReadUInt32();
      if (m_Signature != NSignature::kCentralFileHeader)
        break;
    }
    
    cdSize = (m_Position - 4) - cdAbsOffset;
    needSetBase = true;
    numCdItems = cdItems.Size();

    if (!cdItems.IsEmpty())
    {
      ArcInfo.CdWasRead = true;
      ArcInfo.FirstItemRelatOffset = cdItems[0].LocalHeaderPos;
    }
  }

  CEcd64 ecd64;
  bool isZip64 = false;
  UInt64 ecd64AbsOffset = m_Position - 4;
  if (m_Signature == NSignature::kEcd64)
  {
    IsZip64 = isZip64 = true;
    UInt64 recordSize = ReadUInt64();

    const unsigned kBufSize = kEcd64_MainSize;
    Byte buf[kBufSize];
    SafeReadBytes(buf, kBufSize);
    ecd64.Parse(buf);

    Skip64(recordSize - kEcd64_MainSize);
    m_Signature = ReadUInt32();

    if (ecd64.thisDiskNumber != 0 || ecd64.startCDDiskNumber != 0)
      return E_NOTIMPL;

    if (needSetBase)
    {
      ArcInfo.Base = cdAbsOffset - ecd64.cdStartOffset;
      cdRelatOffset = ecd64.cdStartOffset;
      needSetBase = false;
    }

    if (ecd64.numEntriesInCDOnThisDisk != numCdItems ||
        ecd64.numEntriesInCD != numCdItems ||
        ecd64.cdSize != cdSize ||
        (ecd64.cdStartOffset != cdRelatOffset &&
        (!items.IsEmpty())))
      return S_FALSE;
  }
  if (m_Signature == NSignature::kEcd64Locator)
  {
    if (!isZip64)
      return S_FALSE;
    /* UInt32 startEndCDDiskNumber = */ ReadUInt32();
    UInt64 ecd64RelatOffset = ReadUInt64();
    /* UInt32 numberOfDisks = */ ReadUInt32();
    if (ecd64AbsOffset != ArcInfo.Base + ecd64RelatOffset)
      return S_FALSE;
    m_Signature = ReadUInt32();
  }
  if (m_Signature != NSignature::kEcd)
    return S_FALSE;

  const unsigned kBufSize = kEcdSize - 4;
  Byte buf[kBufSize];
  SafeReadBytes(buf, kBufSize);
  CEcd ecd;
  ecd.Parse(buf);

  COPY_ECD_ITEM_16(thisDiskNumber);
  COPY_ECD_ITEM_16(startCDDiskNumber);
  COPY_ECD_ITEM_16(numEntriesInCDOnThisDisk);
  COPY_ECD_ITEM_16(numEntriesInCD);
  COPY_ECD_ITEM_32(cdSize);
  COPY_ECD_ITEM_32(cdStartOffset);

  if (needSetBase)
  {
    ArcInfo.Base = cdAbsOffset - ecd64.cdStartOffset;
    cdRelatOffset = ecd64.cdStartOffset;
    needSetBase = false;
  }

  if (localsWereRead && (UInt64)ArcInfo.Base != ArcInfo.MarkerPos)
  {
    UInt64 delta = ArcInfo.MarkerPos - ArcInfo.Base;
    for (unsigned i = 0; i < items.Size(); i++)
      items[i].LocalHeaderPos += delta;
  }


  // ---------- merge Central Directory Items ----------

  if (!cdItems.IsEmpty())
  {
    for (unsigned i = 0; i < cdItems.Size(); i++)
    {
      const CItemEx &cdItem = cdItems[i];
      int index = FindItem(items, cdItem.LocalHeaderPos);
      if (index == -1)
      {
        items.Add(cdItem);
        continue;
      }
      CItemEx &item = items[index];
      if (item.Name != cdItem.Name
          // || item.Name.Len() != cdItem.Name.Len()
          || item.PackSize != cdItem.PackSize
          || item.Size != cdItem.Size
          // item.ExtractVersion != cdItem.ExtractVersion
          || !FlagsAreSame(item, cdItem)
          || item.Crc != cdItem.Crc)
        continue;

      // item.LocalHeaderPos = cdItem.LocalHeaderPos;
      // item.Name = cdItem.Name;
      item.MadeByVersion = cdItem.MadeByVersion;
      item.CentralExtra = cdItem.CentralExtra;
      item.InternalAttrib = cdItem.InternalAttrib;
      item.ExternalAttrib = cdItem.ExternalAttrib;
      item.Comment = cdItem.Comment;
      item.FromCentral = cdItem.FromCentral;
    }
  }

  if (ecd64.thisDiskNumber != 0 || ecd64.startCDDiskNumber != 0)
    return E_NOTIMPL;
  
  if (isZip64)
  {
    if (ecd64.numEntriesInCDOnThisDisk != items.Size())
      HeadersError = true;
  }
  else
  {
    // old 7-zip could store 32-bit number of CD items to 16-bit field.
    if ((UInt16)ecd64.numEntriesInCDOnThisDisk != (UInt16)numCdItems ||
        (UInt16)ecd64.numEntriesInCDOnThisDisk != (UInt16)items.Size())
      HeadersError = true;
  }

  ReadBuffer(ArcInfo.Comment, ecd.commentSize);
  _inBufMode = false;
  _inBuffer.Free();

  if (
      (UInt16)ecd64.numEntriesInCD != ((UInt16)numCdItems) ||
      (UInt32)ecd64.cdSize != (UInt32)cdSize ||
      ((UInt32)(ecd64.cdStartOffset) != (UInt32)cdRelatOffset &&
        (!items.IsEmpty())))
  {
    // return S_FALSE;
    HeadersError = true;
  }
  
  // printf("\nOpen OK");
  return S_OK;
}

HRESULT CInArchive::ReadHeaders(CObjectVector<CItemEx> &items, CProgressVirt *progress)
{
  HRESULT res;
  try
  {
    res = ReadHeaders2(items, progress);
  }
  catch (const CInBufferException &e) { res = e.ErrorCode; }
  catch (const CUnexpectEnd &)
  {
    if (items.IsEmpty())
      return S_FALSE;
    UnexpectedEnd = true;
    res = S_OK;
  }
  catch (...)
  {
    _inBufMode = false;
    throw;
  }
  ArcInfo.FinishPos = m_Position;
  _inBufMode = false;
  return res;
}

ISequentialInStream* CInArchive::CreateLimitedStream(UInt64 position, UInt64 size)
{
  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> stream(streamSpec);
  Stream->Seek(ArcInfo.Base + position, STREAM_SEEK_SET, NULL);
  streamSpec->SetStream(Stream);
  streamSpec->Init(size);
  return stream.Detach();
}

}}
