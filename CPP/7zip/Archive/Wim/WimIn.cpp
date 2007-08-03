// Archive/WimIn.cpp

#include "StdAfx.h"

#include "Common/MyCom.h"
#include "Common/IntToString.h"

#include "../../Common/StreamUtils.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/LimitedStreams.h"

#include "../Common/OutStreamWithSha1.h"

#include "../../../../C/CpuArch.h"

#include "WimIn.h"

namespace NArchive{
namespace NWim{

static const int kChunkSizeBits = 15;
static const UInt32 kChunkSize = (1 << kChunkSizeBits);

static HRESULT ReadBytes(ISequentialInStream *inStream, void *data, UInt32 size)
{
  UInt32 realProcessedSize;
  RINOK(ReadStream(inStream, data, size, &realProcessedSize));
  return (realProcessedSize == size) ? S_OK : S_FALSE;
}

#ifdef LITTLE_ENDIAN_UNALIGN
static inline UInt16 GetUInt16FromMem(const Byte *p) { return *(const UInt16 *)p; }
static inline UInt32 GetUInt32FromMem(const Byte *p) { return *(const UInt32 *)p; }
static inline UInt64 GetUInt64FromMem(const Byte *p) { return *(const UInt64 *)p; }
#else
static UInt16 GetUInt16FromMem(const Byte *p) { return p[0] | ((UInt16)p[1] << 8); }
static UInt32 GetUInt32FromMem(const Byte *p) { return p[0] | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16) | ((UInt32)p[3] << 24); }
static UInt64 GetUInt64FromMem(const Byte *p) { return GetUInt32FromMem(p) | ((UInt64)GetUInt32FromMem(p + 4) << 32); }
#endif

static void GetFileTimeFromMem(const Byte *p, FILETIME *ft)
{
  ft->dwLowDateTime = GetUInt32FromMem(p);
  ft->dwHighDateTime = GetUInt32FromMem(p + 4);
}

HRESULT CUnpacker::Unpack(IInStream *inStream, const CResource &resource,
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
  UInt32 sizesBufSize = (UInt32)sizesBufSize64;
  if (sizesBufSize != sizesBufSize64)
    return E_OUTOFMEMORY;
  if (sizesBufSize > sizesBuf.GetCapacity())
  {
    sizesBuf.Free();
    sizesBuf.SetCapacity(sizesBufSize);
  }
  RINOK(ReadBytes(inStream, (Byte *)sizesBuf, sizesBufSize));
  const Byte *p = (const Byte *)sizesBuf;
  
  if (!lzxDecoder)
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
      if (entrySize == 4)
        offset = GetUInt32FromMem(p);
      else
        offset = GetUInt64FromMem(p);
      p += entrySize;
    }
    UInt64 nextOffset = resource.PackSize - sizesBufSize64;
    if (i + 1 < (UInt32)numChunks)
      if (entrySize == 4)
        nextOffset = GetUInt32FromMem(p);
      else
        nextOffset = GetUInt64FromMem(p);
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
    lzxDecoderSpec->SetKeepHistory(false);
    ICompressCoder *coder = (inSize == outSize) ? copyCoder : lzxDecoder;
    RINOK(coder->Code(limitedStreamSpec, outStream, NULL, &outSize64, NULL));
    outProcessed += outSize;
  }
  return S_OK;
}

HRESULT CUnpacker::Unpack(IInStream *inStream, const CResource &resource,
    ISequentialOutStream *outStream, ICompressProgressInfo *progress, Byte *digest)
{
  COutStreamWithSha1 *shaStreamSpec = new COutStreamWithSha1();
  CMyComPtr<ISequentialOutStream> shaStream = shaStreamSpec;
  shaStreamSpec->SetStream(outStream);
  shaStreamSpec->Init(digest != NULL);
  HRESULT result = Unpack(inStream, resource, shaStream, progress);
  if (digest)
    shaStreamSpec->Final(digest);
  return result;
}

static HRESULT UnpackData(IInStream *inStream, const CResource &resource, CByteBuffer &buf, Byte *digest)
{
  size_t size = (size_t)resource.UnpackSize;
  if (size != resource.UnpackSize)
    return E_OUTOFMEMORY;
  buf.Free();
  buf.SetCapacity(size);

  CSequentialOutStreamImp2 *outStreamSpec = new CSequentialOutStreamImp2();
  CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
  outStreamSpec->Init((Byte *)buf, size);

  CUnpacker unpacker;
  return unpacker.Unpack(inStream, resource, outStream, NULL, digest);
}

static const UInt32 kSignatureSize = 8;
static const Byte kSignature[kSignatureSize] = { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 };

static void GetResource(const Byte *p, CResource &res)
{
  res.Flags = p[7];
  res.PackSize = GetUInt64FromMem(p) & (((UInt64)1 << 56) - 1);
  res.Offset = GetUInt64FromMem(p + 8);
  res.UnpackSize = GetUInt64FromMem(p + 16);
}

static void GetStream(const Byte *p, CStreamInfo &s)
{
  GetResource(p, s.Resource);
  s.PartNumber = GetUInt16FromMem(p + 24);
  s.RefCount = GetUInt32FromMem(p + 26);
  memcpy(s.Hash, p + 30, kHashSize);
}

static HRESULT ParseDirItem(const Byte *base, size_t pos, size_t size, 
    const UString &prefix, CObjectVector<CItem> &items)
{
  for (;;)
  {
    if (pos + 8 > size)
      return S_FALSE;
    const Byte *p = base + pos;
    UInt64 length = GetUInt64FromMem(p);
    if (length == 0)
      return S_OK;
    if (pos + 102 > size || pos + length + 8 > size || length > ((UInt64)1 << 62))
      return S_FALSE;
    CItem item;
    item.Attributes = GetUInt32FromMem(p + 8);
    // item.SecurityId = GetUInt32FromMem(p + 0xC);
    UInt64 subdirOffset = GetUInt64FromMem(p + 0x10);
    GetFileTimeFromMem(p + 0x28, &item.CreationTime);
    GetFileTimeFromMem(p + 0x30, &item.LastAccessTime);
    GetFileTimeFromMem(p + 0x38, &item.LastWriteTime);
    memcpy(item.Hash, p + 0x40, kHashSize);

    // UInt16 shortNameLen = GetUInt16FromMem(p + 98);
    UInt16 fileNameLen = GetUInt16FromMem(p + 100);
    
    size_t tempPos = pos + 102;
    if (tempPos + fileNameLen > size)
      return S_FALSE;
    
    wchar_t *sz = item.Name.GetBuffer(prefix.Length() + fileNameLen / 2 + 1);
    MyStringCopy(sz, (const wchar_t *)prefix);
    sz += prefix.Length();
    for (UInt16 i = 0; i + 2 <= fileNameLen; i += 2)
      *sz++ = GetUInt16FromMem(base + tempPos + i);
    *sz++ = '\0';
    item.Name.ReleaseBuffer();
    if (fileNameLen == 0 && item.IsDirectory() && !item.HasStream())
    {
      item.Attributes = 0x10; // some swm archives have system/hidden attributes for root
      item.Name.Delete(item.Name.Length() - 1);
    }
    items.Add(item);
    pos += (size_t)length;
    if (item.IsDirectory() && (subdirOffset != 0))
    {
      if (subdirOffset >= size)
        return S_FALSE;
      RINOK(ParseDirItem(base, (size_t)subdirOffset, size, item.Name + WCHAR_PATH_SEPARATOR, items));
    }
  }
}

static HRESULT ParseDir(const Byte *base, size_t size, 
    const UString &prefix, CObjectVector<CItem> &items)
{
  size_t pos = 0; 
  if (pos + 8 > size)
    return S_FALSE;
  const Byte *p = base + pos;
  UInt32 totalLength = GetUInt32FromMem(p);
  // UInt32 numEntries = GetUInt32FromMem(p + 4);
  pos += 8;
  {
    /*
    CRecordVector<UInt64> entryLens;
    UInt64 sum = 0;
    for (UInt32 i = 0; i < numEntries; i++)
    {
      if (pos + 8 > size)
        return S_FALSE;
      UInt64 len = GetUInt64FromMem(p + pos);
      entryLens.Add(len);
      sum += len;
      pos += 8;
    }
    pos += sum; // skeep security descriptors
    while ((pos & 7) != 0)
      pos++;
    if (pos != totalLength)
      return S_FALSE;
    */
    pos = totalLength;
  }
  return ParseDirItem(base, pos, size, prefix, items);
}

static int CompareHashRefs(const int *p1, const int *p2, void *param)
{
  const CRecordVector<CStreamInfo> &streams = *(const CRecordVector<CStreamInfo> *)param;
  return memcmp(streams[*p1].Hash, streams[*p2].Hash, kHashSize);
}

static int CompareStreamsByPos(const CStreamInfo *p1, const CStreamInfo *p2, void * /* param */)
{
  int res = MyCompare(p1->PartNumber, p2->PartNumber);
  if (res != 0)
    return res;
  return MyCompare(p1->Resource.Offset, p2->Resource.Offset);
}

int CompareItems(void *const *a1, void *const *a2, void * /* param */)
{
  const CItem &i1 = **((const CItem **)a1);
  const CItem &i2 = **((const CItem **)a2);

  if (i1.IsDirectory() != i2.IsDirectory())
    return (i1.IsDirectory()) ? 1 : -1;
  if (i1.IsDirectory())
    return -MyStringCompareNoCase(i1.Name, i2.Name);

  int res = MyCompare(i1.StreamIndex, i2.StreamIndex);
  if (res != 0)
    return res;
  return MyStringCompareNoCase(i1.Name, i2.Name);
}

static int FindHash(const CRecordVector<CStreamInfo> &streams, 
    const CRecordVector<int> &sortedByHash, const Byte *hash)
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

HRESULT ReadHeader(IInStream *inStream, CHeader &h)
{
  const UInt32 kHeaderSizeMax = 0xD0;
  Byte p[kHeaderSizeMax];
  RINOK(ReadBytes(inStream, p, kHeaderSizeMax));
  UInt32 haderSize = GetUInt32FromMem(p + 8);
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  if (haderSize < 0x74)
    return S_FALSE;
  h.Version = GetUInt32FromMem(p + 0x0C);
  h.Flags = GetUInt32FromMem(p + 0x10);
  if (!h.IsSupported())
    return S_FALSE;
  if (GetUInt32FromMem(p + 0x14) != kChunkSize)
    return S_FALSE;
  memcpy(h.Guid, p + 0x18, 16);
  h.PartNumber = GetUInt16FromMem(p + 0x28);
  h.NumParts = GetUInt16FromMem(p + 0x2A);
  int offset = 0x2C;
  if (h.IsNewVersion())
  {
    h.NumImages = GetUInt32FromMem(p + offset);
    offset += 4;
  }
  GetResource(p + offset, h.OffsetResource);
  GetResource(p + offset + 0x18, h.XmlResource);
  GetResource(p + offset + 0x30, h.MetadataResource);
  /*
  if (h.IsNewVersion())
  {
    if (haderSize < 0xD0)
      return S_FALSE;
    GetResource(p + offset + 0x4C, h.IntegrityResource);
    h.BootIndex = GetUInt32FromMem(p + 0x48);
  }
  */
  return S_OK;
}

HRESULT ReadStreams(IInStream *inStream, const CHeader &h, CDatabase &db)
{
  CByteBuffer offsetBuf;
  RINOK(UnpackData(inStream, h.OffsetResource, offsetBuf, NULL));
  for (size_t i = 0; i + kStreamInfoSize <= offsetBuf.GetCapacity(); i += kStreamInfoSize)
  {
    CStreamInfo s;
    GetStream((const Byte *)offsetBuf + i, s);
    if (s.PartNumber == h.PartNumber)
      db.Streams.Add(s);
  }
  return S_OK;
}

HRESULT OpenArchive(IInStream *inStream, const CHeader &h, CByteBuffer &xml, CDatabase &db)
{
  RINOK(UnpackData(inStream, h.XmlResource, xml, NULL));

  RINOK(ReadStreams(inStream, h, db));
  bool needBootMetadata = !h.MetadataResource.IsEmpty();
  if (h.PartNumber == 1)
  {
    int imageIndex = 1;
    for (int j = 0; j < db.Streams.Size(); j++)
    {
      // if (imageIndex > 1) break;
      const CStreamInfo &si = db.Streams[j];
      if (!si.Resource.IsMetadata() || si.PartNumber != h.PartNumber)
        continue;
      Byte hash[kHashSize];
      CByteBuffer metadata;
      RINOK(UnpackData(inStream, si.Resource, metadata, hash));
      if (memcmp(hash, si.Hash, kHashSize) != 0)
        return S_FALSE;
      wchar_t sz[32];
      ConvertUInt64ToString(imageIndex++, sz);
      UString s = sz;
      s += WCHAR_PATH_SEPARATOR; 
      RINOK(ParseDir(metadata, metadata.GetCapacity(), s, db.Items));
      if (needBootMetadata)
        if (h.MetadataResource.Offset == si.Resource.Offset)
          needBootMetadata = false;
    }
  }
  
  if (needBootMetadata)
  {
    CByteBuffer metadata;
    RINOK(UnpackData(inStream, h.MetadataResource, metadata, NULL));
    RINOK(ParseDir(metadata, metadata.GetCapacity(), L"0" WSTRING_PATH_SEPARATOR, db.Items));
  }
  return S_OK;
}


HRESULT SortDatabase(CDatabase &db)
{
  db.Streams.Sort(CompareStreamsByPos, NULL);

  {
    CRecordVector<int> sortedByHash;
    {
      for (int j = 0; j < db.Streams.Size(); j++)
        sortedByHash.Add(j);
      sortedByHash.Sort(CompareHashRefs, &db.Streams);
    }
    
    for (int i = 0; i < db.Items.Size(); i++)
    {
      CItem &item = db.Items[i];
      item.StreamIndex = -1;
      if (item.HasStream())
        item.StreamIndex = FindHash(db.Streams, sortedByHash, item.Hash);
    }
  }

  {
    CRecordVector<bool> used;
    int j;
    for (j = 0; j < db.Streams.Size(); j++)
    {
      const CStreamInfo &s = db.Streams[j];
      used.Add(s.Resource.IsMetadata() && s.PartNumber == 1);
    }
    for (int i = 0; i < db.Items.Size(); i++)
    {
      CItem &item = db.Items[i];
      if (item.StreamIndex >= 0)
        used[item.StreamIndex] = true;
    }
    for (j = 0; j < db.Streams.Size(); j++)
      if (!used[j])
      {
        CItem item;
        item.StreamIndex = j;
        item.HasMetadata = false;
        db.Items.Add(item);
    }
  }
  
  db.Items.Sort(CompareItems, NULL);
  return S_OK;
}

}}
