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

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

namespace NArchive{
namespace NWim{

static const int kChunkSizeBits = 15;
static const UInt32 kChunkSize = (1 << kChunkSizeBits);

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
    for (int i = 0; i < kMainTableSize; i += 2)
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

static void GetFileTimeFromMem(const Byte *p, FILETIME *ft)
{
  ft->dwLowDateTime = Get32(p);
  ft->dwHighDateTime = Get32(p + 4);
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

  CSequentialOutStreamImp2 *outStreamSpec = new CSequentialOutStreamImp2();
  CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
  outStreamSpec->Init((Byte *)buf, size);

  CUnpacker unpacker;
  return unpacker.Unpack(inStream, resource, lzxMode, outStream, NULL, digest);
}

static const UInt32 kSignatureSize = 8;
static const Byte kSignature[kSignatureSize] = { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 };

void CResource::Parse(const Byte *p)
{
  Flags = p[7];
  PackSize = Get64(p) & (((UInt64)1 << 56) - 1);
  Offset = Get64(p + 8);
  UnpackSize = Get64(p + 16);
}

#define GetResource(p, res) res.Parse(p)

static void GetStream(const Byte *p, CStreamInfo &s)
{
  s.Resource.Parse(p);
  s.PartNumber = Get16(p + 24);
  s.RefCount = Get32(p + 26);
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
    UInt64 length = Get64(p);
    if (length == 0)
      return S_OK;
    if (pos + 102 > size || pos + length + 8 > size || length > ((UInt64)1 << 62))
      return S_FALSE;
    CItem item;
    item.Attrib = Get32(p + 8);
    // item.SecurityId = Get32(p + 0xC);
    UInt64 subdirOffset = Get64(p + 0x10);
    GetFileTimeFromMem(p + 0x28, &item.CTime);
    GetFileTimeFromMem(p + 0x30, &item.ATime);
    GetFileTimeFromMem(p + 0x38, &item.MTime);
    memcpy(item.Hash, p + 0x40, kHashSize);

    // UInt16 shortNameLen = Get16(p + 98);
    UInt16 fileNameLen = Get16(p + 100);
    
    size_t tempPos = pos + 102;
    if (tempPos + fileNameLen > size)
      return S_FALSE;
    
    wchar_t *sz = item.Name.GetBuffer(prefix.Length() + fileNameLen / 2 + 1);
    MyStringCopy(sz, (const wchar_t *)prefix);
    sz += prefix.Length();
    for (UInt16 i = 0; i + 2 <= fileNameLen; i += 2)
      *sz++ = Get16(base + tempPos + i);
    *sz++ = '\0';
    item.Name.ReleaseBuffer();
    if (fileNameLen == 0 && item.isDir() && !item.HasStream())
    {
      item.Attrib = 0x10; // some swm archives have system/hidden attributes for root
      item.Name.Delete(item.Name.Length() - 1);
    }
    items.Add(item);
    pos += (size_t)length;
    if (item.isDir() && (subdirOffset != 0))
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
  UInt32 totalLength = Get32(p);
  // UInt32 numEntries = Get32(p + 4);
  pos += 8;
  {
    /*
    CRecordVector<UInt64> entryLens;
    UInt64 sum = 0;
    for (UInt32 i = 0; i < numEntries; i++)
    {
      if (pos + 8 > size)
        return S_FALSE;
      UInt64 len = Get64(p + pos);
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

  if (i1.isDir() != i2.isDir())
    return (i1.isDir()) ? 1 : -1;
  if (i1.isDir())
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

HRESULT CHeader::Parse(const Byte *p)
{
  UInt32 haderSize = Get32(p + 8);
  if (haderSize < 0x74)
    return S_FALSE;
  Version = Get32(p + 0x0C);
  Flags = Get32(p + 0x10);
  if (!IsSupported())
    return S_FALSE;
  UInt32 chunkSize = Get32(p + 0x14);
  if (chunkSize != kChunkSize && chunkSize != 0)
    return S_FALSE;
  memcpy(Guid, p + 0x18, 16);
  PartNumber = Get16(p + 0x28);
  NumParts = Get16(p + 0x2A);
  int offset = 0x2C;
  if (IsNewVersion())
  {
    NumImages = Get32(p + offset);
    offset += 4;
  }
  GetResource(p + offset, OffsetResource);
  GetResource(p + offset + 0x18, XmlResource);
  GetResource(p + offset + 0x30, MetadataResource);
  /*
  if (IsNewVersion())
  {
    if (haderSize < 0xD0)
      return S_FALSE;
    IntegrityResource.Parse(p + offset + 0x4C);
    BootIndex = Get32(p + 0x48);
  }
  */
  return S_OK;
}

HRESULT ReadHeader(IInStream *inStream, CHeader &h)
{
  const UInt32 kHeaderSizeMax = 0xD0;
  Byte p[kHeaderSizeMax];
  RINOK(ReadStream_FALSE(inStream, p, kHeaderSizeMax));
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  return h.Parse(p);
}

HRESULT ReadStreams(IInStream *inStream, const CHeader &h, CDatabase &db)
{
  CByteBuffer offsetBuf;
  RINOK(UnpackData(inStream, h.OffsetResource, h.IsLzxMode(), offsetBuf, NULL));
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
  RINOK(UnpackData(inStream, h.XmlResource, h.IsLzxMode(), xml, NULL));

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
      RINOK(UnpackData(inStream, si.Resource, h.IsLzxMode(), metadata, hash));
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
    RINOK(UnpackData(inStream, h.MetadataResource, h.IsLzxMode(), metadata, NULL));
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
