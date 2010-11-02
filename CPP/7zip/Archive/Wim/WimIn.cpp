// Archive/WimIn.cpp

#include "StdAfx.h"

#include "../../../../C/CpuArch.h"

#include "Common/IntToString.h"

#include "../../Common/StreamUtils.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/LimitedStreams.h"

#include "../Common/OutStreamWithSha1.h"

#include "WimIn.h"

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

namespace NArchive {
namespace NWim {

namespace NXpress {

class CDecoderFlusher
{
  CDecoder *m_Decoder;
public:
  bool NeedFlush;
  CDecoderFlusher(CDecoder *decoder): m_Decoder(decoder), NeedFlush(true) {}
  ~CDecoderFlusher()
  {
    if (NeedFlush)
      m_Decoder->Flush();
    m_Decoder->ReleaseStreams();
  }
};

HRESULT CDecoder::CodeSpec(UInt32 outSize)
{
  {
    Byte levels[kMainTableSize];
    for (unsigned i = 0; i < kMainTableSize; i += 2)
    {
      Byte b = m_InBitStream.DirectReadByte();
      levels[i] = b & 0xF;
      levels[i + 1] = b >> 4;
    }
    if (!m_MainDecoder.SetCodeLengths(levels))
      return S_FALSE;
  }

  while (outSize > 0)
  {
    UInt32 number = m_MainDecoder.DecodeSymbol(&m_InBitStream);
    if (number < 256)
    {
      m_OutWindowStream.PutByte((Byte)number);
      outSize--;
    }
    else
    {
      if (number >= kMainTableSize)
        return S_FALSE;
      UInt32 posLenSlot = number - 256;
      UInt32 posSlot = posLenSlot / kNumLenSlots;
      UInt32 len = posLenSlot % kNumLenSlots;
      UInt32 distance = (1 << posSlot) - 1 + m_InBitStream.ReadBits(posSlot);
      
      if (len == kNumLenSlots - 1)
      {
        len = m_InBitStream.DirectReadByte();
        if (len == 0xFF)
        {
          len = m_InBitStream.DirectReadByte();
          len |= (UInt32)m_InBitStream.DirectReadByte() << 8;
        }
        else
          len += kNumLenSlots - 1;
      }
      
      len += kMatchMinLen;
      UInt32 locLen = (len <= outSize ? len : outSize);

      if (!m_OutWindowStream.CopyBlock(distance, locLen))
        return S_FALSE;
      
      len -= locLen;
      outSize -= locLen;
      if (len != 0)
        return S_FALSE;
    }
  }
  return S_OK;
}

const UInt32 kDictSize = (1 << kNumPosSlots);

HRESULT CDecoder::CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt32 outSize)
{
  if (!m_OutWindowStream.Create(kDictSize) || !m_InBitStream.Create(1 << 16))
    return E_OUTOFMEMORY;

  CDecoderFlusher flusher(this);

  m_InBitStream.SetStream(inStream);
  m_OutWindowStream.SetStream(outStream);
  m_InBitStream.Init();
  m_OutWindowStream.Init(false);

  RINOK(CodeSpec(outSize));

  flusher.NeedFlush = false;
  return Flush();
}

HRESULT CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt32 outSize)
{
  try { return CodeReal(inStream, outStream, outSize); }
  catch(const CInBufferException &e) { return e.ErrorCode; } \
  catch(const CLzOutWindowException &e) { return e.ErrorCode; }
  catch(...) { return S_FALSE; }
}

}

HRESULT CUnpacker::Unpack(IInStream *inStream, const CResource &resource, bool lzxMode,
    ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  RINOK(inStream->Seek(resource.Offset, STREAM_SEEK_SET, NULL));

  CLimitedSequentialInStream *limitedStreamSpec = new CLimitedSequentialInStream();
  CMyComPtr<ISequentialInStream> limitedStream = limitedStreamSpec;
  limitedStreamSpec->SetStream(inStream);

  if (!copyCoder)
  {
    copyCoderSpec = new NCompress::CCopyCoder;
    copyCoder = copyCoderSpec;
  }
  if (!resource.IsCompressed())
  {
    if (resource.PackSize != resource.UnpackSize)
      return S_FALSE;
    limitedStreamSpec->Init(resource.PackSize);
    return copyCoder->Code(limitedStreamSpec, outStream, NULL, NULL, progress);
  }
  if (resource.UnpackSize == 0)
    return S_OK;
  UInt64 numChunks = (resource.UnpackSize + kChunkSize - 1) >> kChunkSizeBits;
  unsigned entrySize = ((resource.UnpackSize > (UInt64)1 << 32) ? 8 : 4);
  UInt64 sizesBufSize64 = entrySize * (numChunks - 1);
  size_t sizesBufSize = (size_t)sizesBufSize64;
  if (sizesBufSize != sizesBufSize64)
    return E_OUTOFMEMORY;
  if (sizesBufSize > sizesBuf.GetCapacity())
  {
    sizesBuf.Free();
    sizesBuf.SetCapacity(sizesBufSize);
  }
  RINOK(ReadStream_FALSE(inStream, (Byte *)sizesBuf, sizesBufSize));
  const Byte *p = (const Byte *)sizesBuf;
  
  if (lzxMode && !lzxDecoder)
  {
    lzxDecoderSpec = new NCompress::NLzx::CDecoder(true);
    lzxDecoder = lzxDecoderSpec;
    RINOK(lzxDecoderSpec->SetParams(kChunkSizeBits));
  }
  
  UInt64 baseOffset = resource.Offset + sizesBufSize64;
  UInt64 outProcessed = 0;
  for (UInt32 i = 0; i < (UInt32)numChunks; i++)
  {
    UInt64 offset = 0;
    if (i > 0)
    {
      offset = (entrySize == 4) ? Get32(p): Get64(p);
      p += entrySize;
    }
    UInt64 nextOffset = resource.PackSize - sizesBufSize64;
    if (i + 1 < (UInt32)numChunks)
      nextOffset = (entrySize == 4) ? Get32(p): Get64(p);
    if (nextOffset < offset)
      return S_FALSE;

    RINOK(inStream->Seek(baseOffset + offset, STREAM_SEEK_SET, NULL));
    UInt64 inSize = nextOffset - offset;
    limitedStreamSpec->Init(inSize);

    if (progress)
    {
      RINOK(progress->SetRatioInfo(&offset, &outProcessed));
    }
    
    UInt32 outSize = kChunkSize;
    if (outProcessed + outSize > resource.UnpackSize)
      outSize = (UInt32)(resource.UnpackSize - outProcessed);
    UInt64 outSize64 = outSize;
    if (inSize == outSize)
    {
      RINOK(copyCoder->Code(limitedStreamSpec, outStream, NULL, &outSize64, NULL));
    }
    else
    {
      if (lzxMode)
      {
        lzxDecoderSpec->SetKeepHistory(false);
        RINOK(lzxDecoder->Code(limitedStreamSpec, outStream, NULL, &outSize64, NULL));
      }
      else
      {
        RINOK(xpressDecoder.Code(limitedStreamSpec, outStream, outSize));
      }
    }
    outProcessed += outSize;
  }
  return S_OK;
}

HRESULT CUnpacker::Unpack(IInStream *inStream, const CResource &resource, bool lzxMode,
    ISequentialOutStream *outStream, ICompressProgressInfo *progress, Byte *digest)
{
  COutStreamWithSha1 *shaStreamSpec = new COutStreamWithSha1();
  CMyComPtr<ISequentialOutStream> shaStream = shaStreamSpec;
  shaStreamSpec->SetStream(outStream);
  shaStreamSpec->Init(digest != NULL);
  HRESULT result = Unpack(inStream, resource, lzxMode, shaStream, progress);
  if (digest)
    shaStreamSpec->Final(digest);
  return result;
}

static HRESULT UnpackData(IInStream *inStream, const CResource &resource, bool lzxMode, CByteBuffer &buf, Byte *digest)
{
  size_t size = (size_t)resource.UnpackSize;
  if (size != resource.UnpackSize)
    return E_OUTOFMEMORY;
  buf.Free();
  buf.SetCapacity(size);

  CBufPtrSeqOutStream *outStreamSpec = new CBufPtrSeqOutStream();
  CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
  outStreamSpec->Init((Byte *)buf, size);

  CUnpacker unpacker;
  return unpacker.Unpack(inStream, resource, lzxMode, outStream, NULL, digest);
}

void CResource::Parse(const Byte *p)
{
  Flags = p[7];
  PackSize = Get64(p) & (((UInt64)1 << 56) - 1);
  Offset = Get64(p + 8);
  UnpackSize = Get64(p + 16);
}

#define GetResource(p, res) res.Parse(p)

static void GetStream(bool oldVersion, const Byte *p, CStreamInfo &s)
{
  s.Resource.Parse(p);
  if (oldVersion)
  {
    s.PartNumber = 1;
    s.Id = Get32(p + 24);
    s.RefCount = Get32(p + 28);
    memcpy(s.Hash, p + 32, kHashSize);
  }
  else
  {
    s.PartNumber = Get16(p + 24);
    s.RefCount = Get32(p + 26);
    memcpy(s.Hash, p + 30, kHashSize);
  }
}

static const wchar_t *kLongPath = L"[LongPath]";

UString CDatabase::GetItemPath(const int index1) const
{
  int size = 0;
  int index = index1;
  int newLevel;
  for (newLevel = 0;; newLevel = 1)
  {
    const CItem &item = Items[index];
    index = item.Parent;
    if (index >= 0 || !SkipRoot)
      size += item.Name.Length() + newLevel;
    if (index < 0)
      break;
    if ((UInt32)size >= ((UInt32)1 << 16))
      return kLongPath;
  }

  wchar_t temp[16];
  int imageLen = 0;
  if (ShowImageNumber)
  {
    ConvertUInt32ToString(-1 - index, temp);
    imageLen = MyStringLen(temp);
    size += imageLen + 1;
  }
  if ((UInt32)size >= ((UInt32)1 << 16))
    return kLongPath;
  
  UString path;
  wchar_t *s = path.GetBuffer(size);
  s[size] = 0;
  if (ShowImageNumber)
  {
    memcpy(s, temp, imageLen * sizeof(wchar_t));
    s[imageLen] = WCHAR_PATH_SEPARATOR;
  }

  index = index1;
  
  for (newLevel = 0;; newLevel = 1)
  {
    const CItem &item = Items[index];
    index = item.Parent;
    if (index >= 0 || !SkipRoot)
    {
      if (newLevel)
        s[--size] = WCHAR_PATH_SEPARATOR;
      size -= item.Name.Length();
      memcpy(s + size, item.Name, sizeof(wchar_t) * item.Name.Length());
    }
    if (index < 0)
    {
      path.ReleaseBuffer();
      return path;
    }
  }
}

static void GetFileTimeFromMem(const Byte *p, FILETIME *ft)
{
  ft->dwLowDateTime = Get32(p);
  ft->dwHighDateTime = Get32(p + 4);
}

static HRESULT ReadName(const Byte *p, int size, UString &dest)
{
  if (size == 0)
    return S_OK;
  if (Get16(p + size) != 0)
    return S_FALSE;
  wchar_t *s = dest.GetBuffer(size / 2);
  for (int i = 0; i <= size; i += 2)
    *s++ = Get16(p + i);
  dest.ReleaseBuffer();
  return S_OK;
}

HRESULT CDatabase::ParseDirItem(size_t pos, int parent)
{
  if ((pos & 7) != 0)
    return S_FALSE;
  
  int prevIndex = -1;
  for (int numItems = 0;; numItems++)
  {
    if (OpenCallback)
    {
      UInt64 numFiles = Items.Size();
      if ((numFiles & 0x3FF) == 0)
      {
        RINOK(OpenCallback->SetCompleted(&numFiles, NULL));
      }
    }
    size_t rem = DirSize - pos;
    if (pos < DirStartOffset || pos > DirSize || rem < 8)
      return S_FALSE;
    const Byte *p = DirData + pos;
    UInt64 len = Get64(p);
    if (len == 0)
    {
      if (parent < 0 && numItems != 1)
        SkipRoot = false;
      DirProcessed += 8;
      return S_OK;
    }
    if ((len & 7) != 0 || rem < len)
      return S_FALSE;
    if (!IsOldVersion)
      if (len < 0x28)
        return S_FALSE;
    DirProcessed += (size_t)len;
    if (DirProcessed > DirSize)
      return S_FALSE;
    int extraOffset = 0;
    if (IsOldVersion)
    {
      if (len < 0x40 || (/* Get32(p + 12) == 0 && */ Get32(p + 0x14) != 0))
      {
        extraOffset = 0x10;
      }
    }
    else if (Get64(p + 8) == 0)
      extraOffset = 0x24;
    if (extraOffset)
    {
      if (prevIndex == -1)
        return S_FALSE;
      UInt32 fileNameLen = Get16(p + extraOffset);
      if ((fileNameLen & 1) != 0)
        return S_FALSE;
      /* Probably different versions of ImageX can use different number of
         additional ZEROs. So we don't use exact check. */
      UInt32 fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);
      if (((extraOffset + 2 + fileNameLen2 + 6) & ~7) > len)
        return S_FALSE;
      
      UString name;
      RINOK(ReadName(p + extraOffset + 2, fileNameLen, name));

      CItem &prevItem = Items[prevIndex];
      if (name.IsEmpty() && !prevItem.HasStream())
      {
        if (IsOldVersion)
          prevItem.Id = Get32(p + 8);
        else
          memcpy(prevItem.Hash, p + 0x10, kHashSize);
      }
      else
      {
        CItem item;
        item.Name = prevItem.Name + L':' + name;
        item.CTime = prevItem.CTime;
        item.ATime = prevItem.ATime;
        item.MTime = prevItem.MTime;
        if (IsOldVersion)
        {
          item.Id = Get32(p + 8);
          memset(item.Hash, 0, kHashSize);
        }
        else
          memcpy(item.Hash, p + 0x10, kHashSize);
        item.Attrib = 0;
        item.Order = Order++;
        item.Parent = parent;
        Items.Add(item);
      }
      pos += (size_t)len;
      continue;
    }

    UInt32 dirRecordSize = IsOldVersion ? kDirRecordSizeOld : kDirRecordSize;
    if (len < dirRecordSize)
      return S_FALSE;

    CItem item;
    item.Attrib = Get32(p + 8);
    // item.SecurityId = Get32(p + 0xC);
    UInt64 subdirOffset = Get64(p + 0x10);
    UInt32 timeOffset = IsOldVersion ? 0x18: 0x28;
    GetFileTimeFromMem(p + timeOffset,      &item.CTime);
    GetFileTimeFromMem(p + timeOffset + 8,  &item.ATime);
    GetFileTimeFromMem(p + timeOffset + 16, &item.MTime);
    if (IsOldVersion)
    {
      item.Id = Get32(p + 0x10);
      memset(item.Hash, 0, kHashSize);
    }
    else
    {
      memcpy(item.Hash, p + 0x40, kHashSize);
    }
    // UInt32 numStreams = Get16(p + dirRecordSize - 6);
    UInt32 shortNameLen = Get16(p + dirRecordSize - 4);
    UInt32 fileNameLen = Get16(p + dirRecordSize - 2);

    if ((shortNameLen & 1) != 0 || (fileNameLen & 1) != 0)
      return S_FALSE;

    UInt32 shortNameLen2 = (shortNameLen == 0 ? shortNameLen : shortNameLen + 2);
    UInt32 fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);

    if (((dirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~7) > len)
      return S_FALSE;
    p += dirRecordSize;
    
    RINOK(ReadName(p, fileNameLen, item.Name));
    RINOK(ReadName(p + fileNameLen2, shortNameLen, item.ShortName));

    if (parent < 0 && (shortNameLen || fileNameLen || !item.IsDir()))
      SkipRoot = false;

    /*
    // there are some extra data for some files.
    p -= dirRecordSize;
    p += ((dirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~7);
    if (((dirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~7) != len)
      p = p;
    */

    /*
    if (parent >= 0)
    {
      UString s = GetItemPath(parent) + L"\\" + item.Name;
      printf("\n%s %8x %S", item.IsDir() ? "D" : " ", (int)subdirOffset, (const wchar_t *)s);
    }
    */

    if (fileNameLen == 0 && item.IsDir() && !item.HasStream())
      item.Attrib = 0x10; // some swm archives have system/hidden attributes for root

    item.Parent = parent;
    prevIndex = Items.Add(item);
    if (item.IsDir() && subdirOffset != 0)
    {
      RINOK(ParseDirItem((size_t)subdirOffset, prevIndex));
    }
    Items[prevIndex].Order = Order++;
    pos += (size_t)len;
  }
}

HRESULT CDatabase::ParseImageDirs(const CByteBuffer &buf, int parent)
{
  DirData = buf;
  DirSize = buf.GetCapacity();

  size_t pos = 0;
  if (DirSize < 8)
    return S_FALSE;
  const Byte *p = DirData;
  UInt32 totalLength = Get32(p);
  if (IsOldVersion)
  {
    for (pos = 4;; pos += 8)
    {
      if (pos + 4 > DirSize)
        return S_FALSE;
      UInt32 n = Get32(p + pos);
      if (n == 0)
        break;
      if (pos + 8 > DirSize)
        return S_FALSE;
      totalLength += Get32(p + pos + 4);
      if (totalLength > DirSize)
        return S_FALSE;
    }
    pos += totalLength + 4;
    pos = (pos + 7) & ~(size_t)7;
    if (pos > DirSize)
      return S_FALSE;
  }
  else
  {

  // UInt32 numEntries = Get32(p + 4);
  pos += 8;
  {
    /*
    CRecordVector<UInt64> entryLens;
    UInt64 sum = 0;
    for (UInt32 i = 0; i < numEntries; i++)
    {
      if (pos + 8 > DirSize)
        return S_FALSE;
      UInt64 len = Get64(p + pos);
      entryLens.Add(len);
      sum += len;
      pos += 8;
    }
    pos += (size_t)sum; // skip security descriptors
    while ((pos & 7) != 0)
      pos++;
    if (pos != totalLength)
      return S_FALSE;
    */
    if (totalLength == 0)
      pos = 8;
    else if (totalLength < 8)
      return S_FALSE;
    else
      pos = totalLength;
  }
  }
  DirStartOffset = DirProcessed = pos;
  RINOK(ParseDirItem(pos, parent));
  if (DirProcessed == DirSize)
    return S_OK;
  /* Original program writes additional 8 bytes (END_OF_ROOT_FOLDER), but
     reference to that folder is empty */
  if (DirProcessed == DirSize - 8 && DirProcessed - DirStartOffset == 112 &&
      Get64(p + DirSize - 8) == 0)
    return S_OK;
  return S_FALSE;
}

HRESULT CHeader::Parse(const Byte *p)
{
  UInt32 headerSize = Get32(p + 8);
  Version = Get32(p + 0x0C);
  Flags = Get32(p + 0x10);
  if (!IsSupported())
    return S_FALSE;
  ChunkSize = Get32(p + 0x14);
  if (ChunkSize != kChunkSize && ChunkSize != 0)
    return S_FALSE;
  int offset;
  if (IsOldVersion())
  {
    if (headerSize != 0x60)
      return S_FALSE;
    memset(Guid, 0, 16);
    offset = 0x18;
    PartNumber = 1;
    NumParts = 1;
  }
  else
  {
    if (headerSize < 0x74)
      return S_FALSE;
    memcpy(Guid, p + 0x18, 16);
    PartNumber = Get16(p + 0x28);
    NumParts = Get16(p + 0x2A);
    offset = 0x2C;
    if (IsNewVersion())
    {
      NumImages = Get32(p + offset);
      offset += 4;
    }
  }
  GetResource(p + offset, OffsetResource);
  GetResource(p + offset + 0x18, XmlResource);
  GetResource(p + offset + 0x30, MetadataResource);
  if (IsNewVersion())
  {
    if (headerSize < 0xD0)
      return S_FALSE;
    BootIndex = Get32(p + 0x48);
    IntegrityResource.Parse(p + offset + 0x4C);
  }
  return S_OK;
}

const Byte kSignature[kSignatureSize] = { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 };

HRESULT ReadHeader(IInStream *inStream, CHeader &h)
{
  Byte p[kHeaderSizeMax];
  RINOK(ReadStream_FALSE(inStream, p, kHeaderSizeMax));
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  return h.Parse(p);
}

static HRESULT ReadStreams(bool oldVersion, IInStream *inStream, const CHeader &h, CDatabase &db)
{
  CByteBuffer offsetBuf;
  RINOK(UnpackData(inStream, h.OffsetResource, h.IsLzxMode(), offsetBuf, NULL));
  size_t i;
  size_t streamInfoSize = oldVersion ? kStreamInfoSize + 2 : kStreamInfoSize;
  for (i = 0; offsetBuf.GetCapacity() - i >= streamInfoSize; i += streamInfoSize)
  {
    CStreamInfo s;
    GetStream(oldVersion, (const Byte *)offsetBuf + i, s);
    if (s.PartNumber == h.PartNumber)
      db.Streams.Add(s);
  }
  return (i == offsetBuf.GetCapacity()) ? S_OK : S_FALSE;
}

static bool IsEmptySha(const Byte *data)
{
  for (int i = 0; i < kHashSize; i++)
    if (data[i] != 0)
      return false;
  return true;
}

HRESULT CDatabase::Open(IInStream *inStream, const CHeader &h, CByteBuffer &xml, IArchiveOpenCallback *openCallback)
{
  OpenCallback = openCallback;
  IsOldVersion = h.IsOldVersion();
  RINOK(UnpackData(inStream, h.XmlResource, h.IsLzxMode(), xml, NULL));
  RINOK(ReadStreams(h.IsOldVersion(), inStream, h, *this));
  bool needBootMetadata = !h.MetadataResource.IsEmpty();
  Order = 0;
  if (h.PartNumber == 1)
  {
    int imageIndex = 1;
    for (int i = 0; i < Streams.Size(); i++)
    {
      // if (imageIndex > 1) break;
      const CStreamInfo &si = Streams[i];
      if (!si.Resource.IsMetadata() || si.PartNumber != h.PartNumber)
        continue;
      Byte hash[kHashSize];
      CByteBuffer metadata;
      RINOK(UnpackData(inStream, si.Resource, h.IsLzxMode(), metadata, hash));
      if (memcmp(hash, si.Hash, kHashSize) != 0 &&
          !(h.IsOldVersion() && IsEmptySha(si.Hash)))
        return S_FALSE;
      NumImages++;
      RINOK(ParseImageDirs(metadata, -(int)(++imageIndex)));
      if (needBootMetadata)
        if (h.MetadataResource.Offset == si.Resource.Offset)
          needBootMetadata = false;
    }
  }
  
  if (needBootMetadata)
  {
    CByteBuffer metadata;
    RINOK(UnpackData(inStream, h.MetadataResource, h.IsLzxMode(), metadata, NULL));
    RINOK(ParseImageDirs(metadata, -1));
    NumImages++;
  }
  return S_OK;
}


static int CompareStreamsByPos(const CStreamInfo *p1, const CStreamInfo *p2, void * /* param */)
{
  int res = MyCompare(p1->PartNumber, p2->PartNumber);
  if (res != 0)
    return res;
  return MyCompare(p1->Resource.Offset, p2->Resource.Offset);
}

static int CompareIDs(const int *p1, const int *p2, void *param)
{
  const CRecordVector<CStreamInfo> &streams = *(const CRecordVector<CStreamInfo> *)param;
  return MyCompare(streams[*p1].Id, streams[*p2].Id);
}

static int CompareHashRefs(const int *p1, const int *p2, void *param)
{
  const CRecordVector<CStreamInfo> &streams = *(const CRecordVector<CStreamInfo> *)param;
  return memcmp(streams[*p1].Hash, streams[*p2].Hash, kHashSize);
}

static int FindId(const CRecordVector<CStreamInfo> &streams,
    const CIntVector &sortedByHash, UInt32 id)
{
  int left = 0, right = streams.Size();
  while (left != right)
  {
    int mid = (left + right) / 2;
    int streamIndex = sortedByHash[mid];
    UInt32 id2 = streams[streamIndex].Id;
    if (id == id2)
      return streamIndex;
    if (id < id2)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

static int FindHash(const CRecordVector<CStreamInfo> &streams,
    const CIntVector &sortedByHash, const Byte *hash)
{
  int left = 0, right = streams.Size();
  while (left != right)
  {
    int mid = (left + right) / 2;
    int streamIndex = sortedByHash[mid];
    UInt32 i;
    const Byte *hash2 = streams[streamIndex].Hash;
    for (i = 0; i < kHashSize; i++)
      if (hash[i] != hash2[i])
        break;
    if (i == kHashSize)
      return streamIndex;
    if (hash[i] < hash2[i])
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

static int CompareItems(const int *a1, const int *a2, void *param)
{
  const CObjectVector<CItem> &items = ((CDatabase *)param)->Items;
  const CItem &i1 = items[*a1];
  const CItem &i2 = items[*a2];

  if (i1.IsDir() != i2.IsDir())
    return i1.IsDir() ? 1 : -1;
  int res = MyCompare(i1.StreamIndex, i2.StreamIndex);
  if (res != 0)
    return res;
  return MyCompare(i1.Order, i2.Order);
}

HRESULT CDatabase::Sort(bool skipRootDir)
{
  Streams.Sort(CompareStreamsByPos, NULL);

  {
    CIntVector sortedByHash;
    {
      for (int i = 0; i < Streams.Size(); i++)
        sortedByHash.Add(i);
      if (IsOldVersion)
        sortedByHash.Sort(CompareIDs, &Streams);
      else
        sortedByHash.Sort(CompareHashRefs, &Streams);
    }
    
    for (int i = 0; i < Items.Size(); i++)
    {
      CItem &item = Items[i];
      item.StreamIndex = -1;
      if (item.HasStream())
        if (IsOldVersion)
          item.StreamIndex = FindId(Streams, sortedByHash, item.Id);
        else
          item.StreamIndex = FindHash(Streams, sortedByHash, item.Hash);
    }
  }

  {
    CRecordVector<bool> used;
    int i;
    for (i = 0; i < Streams.Size(); i++)
    {
      const CStreamInfo &s = Streams[i];
      used.Add(s.Resource.IsMetadata() && s.PartNumber == 1);
      // used.Add(false);
    }
    for (i = 0; i < Items.Size(); i++)
    {
      CItem &item = Items[i];
      if (item.StreamIndex >= 0)
        used[item.StreamIndex] = true;
    }
    for (i = 0; i < Streams.Size(); i++)
      if (!used[i])
      {
        CItem item;
        item.StreamIndex = i;
        item.HasMetadata = false;
        Items.Add(item);
      }
  }

  SortedItems.Reserve(Items.Size());
  for (int i = (skipRootDir ? 1 : 0); i < Items.Size(); i++)
    SortedItems.Add(i);
  SortedItems.Sort(CompareItems, this);
  return S_OK;
}

}}
