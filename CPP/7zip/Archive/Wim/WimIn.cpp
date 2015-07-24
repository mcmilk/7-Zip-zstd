// Archive/WimIn.cpp

#include "StdAfx.h"

// #include <stdio.h>

#include "../../../../C/CpuArch.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/StringToInt.h"
#include "../../../Common/UTFConvert.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

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
  }
};

HRESULT CDecoder::CodeSpec(UInt32 outSize)
{
  {
    Byte levels[kMainTableSize];
    for (unsigned i = 0; i < kMainTableSize; i += 2)
    {
      Byte b = m_InBitStream.DirectReadByte();
      levels[i] = (Byte)(b & 0xF);
      levels[i + 1] = (Byte)(b >> 4);
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
  sizesBuf.AllocAtLeast(sizesBufSize);
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
  buf.Alloc(size);

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

#define GET_RESOURCE(_p_, res) res.ParseAndUpdatePhySize(_p_, phySize)

static void GetStream(bool oldVersion, const Byte *p, CStreamInfo &s)
{
  s.Resource.Parse(p);
  if (oldVersion)
  {
    s.PartNumber = 1;
    s.Id = Get32(p + 24);
    // printf("\n%d", s.Id);
    p += 28;
  }
  else
  {
    s.PartNumber = Get16(p + 24);
    p += 26;
  }
  s.RefCount = Get32(p);
  memcpy(s.Hash, p + 4, kHashSize);
}


static const char *kLongPath = "[LongPath]";

void CDatabase::GetShortName(unsigned index, NWindows::NCOM::CPropVariant &name) const
{
  const CItem &item = Items[index];
  const CImage &image = Images[item.ImageIndex];
  if (item.Parent < 0 && image.NumEmptyRootItems != 0)
  {
    name.Clear();
    return;
  }
  const Byte *meta = image.Meta + item.Offset +
      (IsOldVersion ? kDirRecordSizeOld : kDirRecordSize);
  UInt32 fileNameLen = Get16(meta - 2);
  UInt32 shortLen = Get16(meta - 4) / 2;
  wchar_t *s = name.AllocBstr(shortLen);
  if (fileNameLen != 0)
    meta += fileNameLen + 2;
  for (UInt32 i = 0; i < shortLen; i++)
    s[i] = Get16(meta + i * 2);
  s[shortLen] = 0;
  // empty shortName has no ZERO at the end ?
}

void CDatabase::GetItemName(unsigned index, NWindows::NCOM::CPropVariant &name) const
{
  const CItem &item = Items[index];
  const CImage &image = Images[item.ImageIndex];
  if (item.Parent < 0 && image.NumEmptyRootItems != 0)
  {
    name = image.RootName;
    return;
  }
  const Byte *meta = image.Meta + item.Offset +
      (item.IsAltStream ?
      (IsOldVersion ? 0x10 : 0x24) :
      (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2));
  UInt32 len = Get16(meta) / 2;
  wchar_t *s = name.AllocBstr(len);
  meta += 2;
  len++;
  for (UInt32 i = 0; i < len; i++)
    s[i] = Get16(meta + i * 2);
}

void CDatabase::GetItemPath(unsigned index1, bool showImageNumber, NWindows::NCOM::CPropVariant &path) const
{
  unsigned size = 0;
  int index = index1;
  int imageIndex = Items[index].ImageIndex;
  const CImage &image = Images[imageIndex];
  
  unsigned newLevel = 0;
  bool needColon = false;

  for (;;)
  {
    const CItem &item = Items[index];
    index = item.Parent;
    if (index >= 0 || image.NumEmptyRootItems == 0)
    {
      const Byte *meta = image.Meta + item.Offset;
      meta += item.IsAltStream ?
          (IsOldVersion ? 0x10 : 0x24) :
          (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2);
      needColon = item.IsAltStream;
      size += Get16(meta) / 2;
      size += newLevel;
      newLevel = 1;
      if (size >= ((UInt32)1 << 15))
      {
        path = kLongPath;
        return;
      }
    }
    if (index < 0)
      break;
  }

  if (showImageNumber)
  {
    size += image.RootName.Len();
    size += newLevel;
  }
  else if (needColon)
    size++;

  wchar_t *s = path.AllocBstr(size);
  s[size] = 0;
  
  if (showImageNumber)
  {
    MyStringCopy(s, (const wchar_t *)image.RootName);
    if (newLevel)
      s[image.RootName.Len()] = (wchar_t)(needColon ? L':' : WCHAR_PATH_SEPARATOR);
  }
  else if (needColon)
    s[0] = L':';

  index = index1;
  wchar_t separator = 0;
  
  for (;;)
  {
    const CItem &item = Items[index];
    index = item.Parent;
    if (index >= 0 || image.NumEmptyRootItems == 0)
    {
      if (separator != 0)
        s[--size] = separator;
      const Byte *meta = image.Meta + item.Offset;
      meta += (item.IsAltStream) ?
          (IsOldVersion ? 0x10: 0x24) :
          (IsOldVersion ? kDirRecordSizeOld - 2 : kDirRecordSize - 2);
      unsigned len = Get16(meta) / 2;
      size -= len;
      wchar_t *dest = s + size;
      meta += 2;
      for (unsigned i = 0; i < len; i++)
        dest[i] = Get16(meta + i * 2);
    }
    if (index < 0)
      return;
    separator = item.IsAltStream ? L':' : WCHAR_PATH_SEPARATOR;
  }
}

static bool IsEmptySha(const Byte *data)
{
  for (unsigned i = 0; i < kHashSize; i++)
    if (data[i] != 0)
      return false;
  return true;
}

// Root folders in OLD archives (ver = 1.10) conatin real items.
// Root folders in NEW archives (ver > 1.11) contain only one folder with empty name.

HRESULT CDatabase::ParseDirItem(size_t pos, int parent)
{
  if ((pos & 7) != 0)
    return S_FALSE;
 
  for (unsigned numItems = 0;; numItems++)
  {
    if (OpenCallback && (Items.Size() & 0xFFFF) == 0)
    {
      UInt64 numFiles = Items.Size();
      RINOK(OpenCallback->SetCompleted(&numFiles, NULL));
    }
    size_t rem = DirSize - pos;
    if (pos < DirStartOffset || pos > DirSize || rem < 8)
      return S_FALSE;
    const Byte *p = DirData + pos;
    UInt64 len = Get64(p);
    if (len == 0)
    {
      if (parent < 0 && numItems != 1)
        Images.Back().NumEmptyRootItems = 0;
      DirProcessed += 8;
      return S_OK;
    }
    if ((len & 7) != 0 || rem < len)
      return S_FALSE;
    DirProcessed += (size_t)len;
    if (DirProcessed > DirSize)
      return S_FALSE;

    UInt32 dirRecordSize = IsOldVersion ? kDirRecordSizeOld : kDirRecordSize;
    if (len < dirRecordSize)
      return S_FALSE;

    CItem item;
    UInt32 attrib = Get32(p + 8);
    item.IsDir = ((attrib & 0x10) != 0);
    UInt64 subdirOffset = Get64(p + 0x10);
    UInt32 numAltStreams = Get16(p + dirRecordSize - 6);
    UInt32 shortNameLen = Get16(p + dirRecordSize - 4);
    UInt32 fileNameLen = Get16(p + dirRecordSize - 2);
    if ((shortNameLen & 1) != 0 || (fileNameLen & 1) != 0)
      return S_FALSE;
    UInt32 shortNameLen2 = (shortNameLen == 0 ? shortNameLen : shortNameLen + 2);
    UInt32 fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);
    if (((dirRecordSize + fileNameLen2 + shortNameLen2 + 6) & ~7) > len)
      return S_FALSE;
    
    p += dirRecordSize;
    
    {
      if (*(const UInt16 *)(p + fileNameLen) != 0)
        return S_FALSE;
      for (UInt32 j = 0; j < fileNameLen; j += 2)
        if (*(const UInt16 *)(p + j) == 0)
          return S_FALSE;
    }
    if (shortNameLen != 0)
    {
      // empty shortName has no ZERO at the end ?
      const Byte *p2 = p + fileNameLen2;
      if (*(const UInt16 *)(p2 + shortNameLen) != 0)
        return S_FALSE;
      for (UInt32 j = 0; j < shortNameLen; j += 2)
        if (*(const UInt16 *)(p2 + j) == 0)
          return S_FALSE;
    }
      
    item.Offset = pos;
    item.Parent = parent;
    item.ImageIndex = Images.Size() - 1;
    unsigned prevIndex = Items.Add(item);

    pos += (size_t)len;

    unsigned numItems2 = Items.Size();
      
    for (UInt32 i = 0; i < numAltStreams; i++)
    {
      size_t rem = DirSize - pos;
      if (pos < DirStartOffset || pos > DirSize || rem < 8)
        return S_FALSE;
      const Byte *p = DirData + pos;
      UInt64 len = Get64(p);
      if (len == 0)
        return S_FALSE;
      if ((len & 7) != 0 || rem < len)
        return S_FALSE;
      if (IsOldVersion)
      {
        if (len < 0x18)
          return S_FALSE;
      }
      else
        if (len < 0x28)
          return S_FALSE;
      DirProcessed += (size_t)len;
      if (DirProcessed > DirSize)
        return S_FALSE;

      unsigned extraOffset = 0;
      if (IsOldVersion)
        extraOffset = 0x10;
      else
      {
        if (Get64(p + 8) != 0)
          return S_FALSE;
        extraOffset = 0x24;
      }
      UInt32 fileNameLen = Get16(p + extraOffset);
      if ((fileNameLen & 1) != 0)
        return S_FALSE;
      /* Probably different versions of ImageX can use different number of
         additional ZEROs. So we don't use exact check. */
      UInt32 fileNameLen2 = (fileNameLen == 0 ? fileNameLen : fileNameLen + 2);
      if (((extraOffset + 2 + fileNameLen2 + 6) & ~7) > len)
        return S_FALSE;
      
      {
        const Byte *p2 = p + extraOffset + 2;
        if (*(const UInt16 *)(p2 + fileNameLen) != 0)
          return S_FALSE;
        for (UInt32 j = 0; j < fileNameLen; j += 2)
          if (*(const UInt16 *)(p2 + j) == 0)
            return S_FALSE;
      }


      /* wim uses alt sreams list, if there is at least one alt stream.
         And alt stream without name is main stream.  */
      
      if (fileNameLen == 0 &&
          (attrib & FILE_ATTRIBUTE_REPARSE_POINT
          || !item.IsDir /* && (IsOldVersion || IsEmptySha(prevMeta + 0x40)) */ ))
      {
        Byte *prevMeta = DirData + item.Offset;
        if (IsOldVersion)
          memcpy(prevMeta + 0x10, p + 8, 4); // It's 32-bit Id
        else
          memcpy(prevMeta + 0x40, p + 0x10, kHashSize);
      }
      else
      {
        ThereAreAltStreams = true;
        CItem item2;
        item2.Offset = pos;
        item2.IsAltStream = true;
        item2.Parent = prevIndex;
        item2.ImageIndex = Images.Size() - 1;
        Items.Add(item2);
      }
      pos += (size_t)len;
    }

    if (parent < 0 && numItems == 0 && shortNameLen == 0 && fileNameLen == 0 && item.IsDir)
    {
      CImage &image = Images.Back();
      image.NumEmptyRootItems = numItems2 - image.StartItem; // Items.Size()
    }

    if (item.IsDir && subdirOffset != 0)
    {
      RINOK(ParseDirItem((size_t)subdirOffset, prevIndex));
    }
  }
}

HRESULT CDatabase::ParseImageDirs(CByteBuffer &buf, int parent)
{
  DirData = buf;
  DirSize = buf.Size();
  if (DirSize < 8)
    return S_FALSE;
  const Byte *p = DirData;
  size_t pos = 0;
  CImage &image = Images.Back();

  if (IsOldVersion)
  {
    // there is no specification about that code
    UInt32 sum = 0;
    image.SecurOffsets.Add(0);
    for (;;)
    {
      if (pos + 8 > DirSize)
        return S_FALSE;
      UInt32 len = Get32(p + pos);
      if (len > DirSize - sum)
        return S_FALSE;
      sum += len;
      image.SecurOffsets.Add(sum);
      UInt32 n = Get32(p + pos + 4); // what does this field mean?
      pos += 8;
      if (n == 0)
        break;
    }
    if (sum > DirSize - pos)
      return S_FALSE;
    FOR_VECTOR (i, image.SecurOffsets)
      image.SecurOffsets[i] += (UInt32)pos;
    pos += sum;
    pos = (pos + 7) & ~(size_t)7;
  }
  else
  {
    UInt32 totalLen = Get32(p);
    if (totalLen == 0)
      pos = 8;
    else
    {
      if (totalLen < 8)
        return S_FALSE;
      UInt32 numEntries = Get32(p + 4);
      pos = 8;
      if (totalLen > DirSize || numEntries > ((totalLen - 8) >> 3))
        return S_FALSE;
      UInt32 sum = (UInt32)pos + numEntries * 8;
      image.SecurOffsets.ClearAndReserve(numEntries + 1);
      image.SecurOffsets.AddInReserved(sum);
      for (UInt32 i = 0; i < numEntries; i++, pos += 8)
      {
        UInt64 len = Get64(p + pos);
        if (len > totalLen - sum)
          return S_FALSE;
        sum += (UInt32)len;
        image.SecurOffsets.AddInReserved(sum);
      }
      pos = sum;
      pos = (pos + 7) & ~(size_t)7;
      if (pos != (((size_t)totalLen + 7) & ~(size_t)7))
        return S_FALSE;
    }
  }
  
  if (pos > DirSize)
    return S_FALSE;
  DirStartOffset = DirProcessed = pos;
  image.StartItem = Items.Size();
  RINOK(ParseDirItem(pos, parent));
  image.NumItems = Items.Size() - image.StartItem;
  if (DirProcessed == DirSize)
    return S_OK;
  /* Original program writes additional 8 bytes (END_OF_ROOT_FOLDER),
     but the reference to that folder is empty */

  // we can't use DirProcessed - DirStartOffset == 112 check if there is alt stream in root
  if (DirProcessed == DirSize - 8 && Get64(p + DirSize - 8) == 0)
    return S_OK;
  return S_FALSE;
}

HRESULT CHeader::Parse(const Byte *p, UInt64 &phySize)
{
  UInt32 headerSize = Get32(p + 8);
  phySize = headerSize;
  Version = Get32(p + 0x0C);
  Flags = Get32(p + 0x10);
  if (!IsSupported())
    return S_FALSE;
  ChunkSize = Get32(p + 0x14);
  if (ChunkSize != kChunkSize && ChunkSize != 0)
    return S_FALSE;
  unsigned offset;
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
  GET_RESOURCE(p + offset       , OffsetResource);
  GET_RESOURCE(p + offset + 0x18, XmlResource);
  GET_RESOURCE(p + offset + 0x30, MetadataResource);
  BootIndex = 0;
  if (IsNewVersion())
  {
    if (headerSize < 0xD0)
      return S_FALSE;
    BootIndex = Get32(p + offset + 0x48);
    GET_RESOURCE(p + offset + 0x4C, IntegrityResource);
  }

  return S_OK;
}

const Byte kSignature[kSignatureSize] = { 'M', 'S', 'W', 'I', 'M', 0, 0, 0 };

HRESULT ReadHeader(IInStream *inStream, CHeader &h, UInt64 &phySize)
{
  Byte p[kHeaderSizeMax];
  RINOK(ReadStream_FALSE(inStream, p, kHeaderSizeMax));
  if (memcmp(p, kSignature, kSignatureSize) != 0)
    return S_FALSE;
  return h.Parse(p, phySize);
}

static HRESULT ReadStreams(IInStream *inStream, const CHeader &h, CDatabase &db)
{
  CByteBuffer offsetBuf;
  RINOK(UnpackData(inStream, h.OffsetResource, h.IsLzxMode(), offsetBuf, NULL));
  size_t i;
  size_t streamInfoSize = h.IsOldVersion() ? kStreamInfoSize + 2 : kStreamInfoSize;
  for (i = 0; offsetBuf.Size() - i >= streamInfoSize; i += streamInfoSize)
  {
    CStreamInfo s;
    GetStream(h.IsOldVersion(), (const Byte *)offsetBuf + i, s);
    if (s.PartNumber == h.PartNumber)
    {
      if (s.Resource.IsMetadata())
      {
        if (s.RefCount == 0)
          return S_FALSE;
        if (s.RefCount > 1)
        {
          s.RefCount--;
          db.DataStreams.Add(s);
        }
        s.RefCount = 1;
        db.MetaStreams.Add(s);
      }
      else
        db.DataStreams.Add(s);
    }
  }
  return (i == offsetBuf.Size()) ? S_OK : S_FALSE;
}

HRESULT CDatabase::OpenXml(IInStream *inStream, const CHeader &h, CByteBuffer &xml)
{
  return UnpackData(inStream, h.XmlResource, h.IsLzxMode(), xml, NULL);
}

static void SetRootNames(CImage &image, unsigned value)
{
  wchar_t temp[16];
  ConvertUInt32ToString(value, temp);
  image.RootName = temp;
  image.RootNameBuf.Alloc(image.RootName.Len() * 2 + 2);
  Byte *p = image.RootNameBuf;
  unsigned len = image.RootName.Len() + 1;
  for (unsigned k = 0; k < len; k++)
  {
    p[k * 2] = (Byte)temp[k];
    p[k * 2 + 1] = 0;
  }
}

HRESULT CDatabase::Open(IInStream *inStream, const CHeader &h, unsigned numItemsReserve, IArchiveOpenCallback *openCallback)
{
  OpenCallback = openCallback;
  IsOldVersion = h.IsOldVersion();
  RINOK(ReadStreams(inStream, h, *this));

  // printf("\nh.PartNumber = %02d", (unsigned)h.PartNumber);
  bool needBootMetadata = !h.MetadataResource.IsEmpty();

  FOR_VECTOR (i, MetaStreams)
  {
    const CStreamInfo &si = MetaStreams[i];
    /*
    printf("\ni = %5d"
      "  Refs = %3d"
      "  Part = %1d"
      "  Offs = %7X"
      "  PackSize = %7X"
      "  Size = %7X"
      "  Flags = %d "
      ,
        i,
        si.RefCount,
        (unsigned)si.PartNumber,
        (unsigned)si.Resource.Offset,
        (unsigned)si.Resource.PackSize,
        (unsigned)si.Resource.UnpackSize,
        (unsigned)si.Resource.Flags
        );
    for (unsigned y = 0; y < 2; y++)
      printf("%02X", (unsigned)si.Hash[y]);
    */

    if (h.PartNumber != 1 || si.PartNumber != h.PartNumber)
      continue;

    Byte hash[kHashSize];
    CImage &image = Images.AddNew();
    SetRootNames(image, Images.Size());
    CByteBuffer &metadata = image.Meta;
    RINOK(UnpackData(inStream, si.Resource, h.IsLzxMode(), metadata, hash));
    if (memcmp(hash, si.Hash, kHashSize) != 0 &&
        !(h.IsOldVersion() && IsEmptySha(si.Hash)))
      return S_FALSE;
    image.NumEmptyRootItems = 0;
    if (Items.IsEmpty())
      Items.ClearAndReserve(numItemsReserve);
    RINOK(ParseImageDirs(metadata, -1));
    if (needBootMetadata)
    {
      bool sameRes = (h.MetadataResource.Offset == si.Resource.Offset);
      if (sameRes)
        needBootMetadata = false;
      bool isBootIndex = (h.BootIndex == (UInt32)Images.Size());
      if (h.IsNewVersion())
      {
        if (sameRes && !isBootIndex)
          return S_FALSE;
        if (isBootIndex && !sameRes)
          return S_FALSE;
      }
    }
  }
  
  if (needBootMetadata)
    return S_FALSE;
  return S_OK;
}


#define RINOZ(x) { int __tt = (x); if (__tt != 0) return __tt; }

static int CompareStreamsByPos(const CStreamInfo *p1, const CStreamInfo *p2, void * /* param */)
{
  RINOZ(MyCompare(p1->PartNumber, p2->PartNumber));
  RINOZ(MyCompare(p1->Resource.Offset, p2->Resource.Offset));
  return MyCompare(p1->Resource.PackSize, p2->Resource.PackSize);
}

static int CompareIDs(const unsigned *p1, const unsigned *p2, void *param)
{
  const CRecordVector<CStreamInfo> &streams = *(const CRecordVector<CStreamInfo> *)param;
  return MyCompare(streams[*p1].Id, streams[*p2].Id);
}

static int CompareHashRefs(const unsigned *p1, const unsigned *p2, void *param)
{
  const CRecordVector<CStreamInfo> &streams = *(const CRecordVector<CStreamInfo> *)param;
  return memcmp(streams[*p1].Hash, streams[*p2].Hash, kHashSize);
}

static int FindId(const CRecordVector<CStreamInfo> &streams,
    const CUIntVector &sortedByHash, UInt32 id)
{
  unsigned left = 0, right = streams.Size();
  while (left != right)
  {
    unsigned mid = (left + right) / 2;
    unsigned streamIndex = sortedByHash[mid];
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
    const CUIntVector &sortedByHash, const Byte *hash)
{
  unsigned left = 0, right = streams.Size();
  while (left != right)
  {
    unsigned mid = (left + right) / 2;
    unsigned streamIndex = sortedByHash[mid];
    const Byte *hash2 = streams[streamIndex].Hash;
    unsigned i;
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

bool CDatabase::ItemHasStream(const CItem &item) const
{
  if (item.ImageIndex < 0)
    return true;
  const Byte *meta = Images[item.ImageIndex].Meta + item.Offset;
  if (IsOldVersion)
  {
    // old wim use same field for file_id and dir_offset;
    if (item.IsDir)
      return false;
    meta += (item.IsAltStream ? 0x8 : 0x10);
    UInt32 id = GetUi32(meta);
    return id != 0;
  }
  meta += (item.IsAltStream ? 0x10 : 0x40);
  for (unsigned i = 0; i < kHashSize; i++)
    if (meta[i] != 0)
      return true;
  return false;
}

static int CompareItems(const unsigned *a1, const unsigned *a2, void *param)
{
  const CRecordVector<CItem> &items = ((CDatabase *)param)->Items;
  const CItem &i1 = items[*a1];
  const CItem &i2 = items[*a2];

  if (i1.IsDir != i2.IsDir)
    return i1.IsDir ? -1 : 1;
  if (i1.IsAltStream != i2.IsAltStream)
    return i1.IsAltStream ? 1 : -1;
  RINOZ(MyCompare(i1.StreamIndex, i2.StreamIndex));
  RINOZ(MyCompare(i1.ImageIndex, i2.ImageIndex));
  return MyCompare(i1.Offset, i2.Offset);
}

HRESULT CDatabase::FillAndCheck()
{
  {
    DataStreams.Sort(CompareStreamsByPos, NULL);
    for (unsigned i = 1; i < DataStreams.Size(); i++)
    {
      const CStreamInfo &s0 = DataStreams[i - 1];
      const CStreamInfo &s1 = DataStreams[i];
      if (s0.PartNumber == s1.PartNumber)
        if (s0.Resource.GetEndLimit() > s1.Resource.Offset)
          return S_FALSE;
    }
  }
  
  {
    CUIntVector sortedByHash;
    {
      unsigned num = DataStreams.Size();
      sortedByHash.ClearAndSetSize(num);
      unsigned *vals = &sortedByHash[0];
      for (unsigned i = 0; i < num; i++)
        vals[i] = i;
      if (IsOldVersion)
        sortedByHash.Sort(CompareIDs, &DataStreams);
      else
        sortedByHash.Sort(CompareHashRefs, &DataStreams);
    }
    
    FOR_VECTOR (i, Items)
    {
      CItem &item = Items[i];
      item.StreamIndex = -1;
      const Byte *hash = Images[item.ImageIndex].Meta + item.Offset;
      if (IsOldVersion)
      {
        if (!item.IsDir)
        {
          hash += (item.IsAltStream ? 0x8 : 0x10);
          UInt32 id = GetUi32(hash);
          if (id != 0)
            item.StreamIndex = FindId(DataStreams, sortedByHash, id);
        }
      }
      /*
      else if (item.IsDir)
      {
        // reparse points can have dirs some dir
      }
      */
      else
      {
        hash += (item.IsAltStream ? 0x10 : 0x40);
        unsigned hi;
        for (hi = 0; hi < kHashSize; hi++)
          if (hash[hi] != 0)
            break;
        if (hi != kHashSize)
        {
          item.StreamIndex = FindHash(DataStreams, sortedByHash, hash);
        }
      }
    }
  }
  {
    CUIntVector refCounts;
    refCounts.ClearAndSetSize(DataStreams.Size());
    unsigned i;

    for (i = 0; i < DataStreams.Size(); i++)
    {
      UInt32 startVal = 0;
      // const CStreamInfo &s = DataStreams[i];
      /*
      if (s.Resource.IsMetadata() && s.PartNumber == 1)
        startVal = 1;
      */
      refCounts[i] = startVal;
    }
    
    for (i = 0; i < Items.Size(); i++)
    {
      int streamIndex = Items[i].StreamIndex;
      if (streamIndex >= 0)
        refCounts[streamIndex]++;
    }
    
    for (i = 0; i < DataStreams.Size(); i++)
    {
      const CStreamInfo &s = DataStreams[i];
      if (s.RefCount != refCounts[i])
      {
        /*
        printf("\ni = %5d  si.Refcount = %d  realRefs = %d size = %6d offset = %6x id = %4d ",
          i, s.RefCount, refCounts[i], (unsigned)s.Resource.UnpackSize, (unsigned)s.Resource.Offset, s.Id);
        */
        // return S_FALSE;
        RefCountError = true;
      }
      if (refCounts[i] == 0)
      {
        CItem item;
        item.Offset = 0;
        item.StreamIndex = i;
        item.ImageIndex = -1;
        ThereAreDeletedStreams = true;
        Items.Add(item);
      }
    }
  }
  return S_OK;
}

HRESULT CDatabase::GenerateSortedItems(int imageIndex, bool showImageNumber)
{
  SortedItems.Clear();
  VirtualRoots.Clear();
  IndexOfUserImage = imageIndex;
  NumExcludededItems = 0;
  ExludedItem = -1;

  if (Images.Size() != 1 && imageIndex < 0)
    showImageNumber = true;

  unsigned startItem = 0;
  unsigned endItem = 0;
  
  if (imageIndex < 0)
  {
    endItem = Items.Size();
    if (Images.Size() == 1)
    {
      IndexOfUserImage = 0;
      const CImage &image = Images[0];
      if (!showImageNumber)
        NumExcludededItems = image.NumEmptyRootItems;
    }
  }
  else if ((unsigned)imageIndex < Images.Size())
  {
    const CImage &image = Images[imageIndex];
    startItem = image.StartItem;
    endItem = startItem + image.NumItems;
    if (!showImageNumber)
      NumExcludededItems = image.NumEmptyRootItems;
  }
  
  if (NumExcludededItems != 0)
  {
    ExludedItem = startItem;
    startItem += NumExcludededItems;
  }

  unsigned num = endItem - startItem;
  SortedItems.ClearAndSetSize(num);
  unsigned i;
  for (i = 0; i < num; i++)
    SortedItems[i] = startItem + i;

  SortedItems.Sort(CompareItems, this);
  for (i = 0; i < SortedItems.Size(); i++)
    Items[SortedItems[i]].IndexInSorted = i;

  if (showImageNumber)
    for (i = 0; i < Images.Size(); i++)
    {
      CImage &image = Images[i];
      if (image.NumEmptyRootItems != 0)
        continue;
      image.VirtualRootIndex = VirtualRoots.Size();
      VirtualRoots.Add(i);
    }
  return S_OK;
}

static void IntVector_SetMinusOne_IfNeed(CIntVector &v, unsigned size)
{
  if (v.Size() == size)
    return;
  v.ClearAndSetSize(size);
  int *vals = &v[0];
  for (unsigned i = 0; i < size; i++)
    vals[i] = -1;
}

HRESULT CDatabase::ExtractReparseStreams(const CObjectVector<CVolume> &volumes, IArchiveOpenCallback *openCallback)
{
  ItemToReparse.Clear();
  ReparseItems.Clear();
  
  // we don't know about Reparse field for OLD WIM format
  if (IsOldVersion)
    return S_OK;

  CIntVector streamToReparse;

  FOR_VECTOR(i, Items)
  {
    // maybe it's better to use sorted items for faster access?
    const CItem &item = Items[i];
    
    if (!item.HasMetadata() || item.IsAltStream)
      continue;
    
    if (item.ImageIndex < 0)
      continue;
    
    const Byte *metadata = Images[item.ImageIndex].Meta + item.Offset;
    
    const UInt32 attrib = Get32(metadata + 8);
    if ((attrib & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
      continue;
    
    if (item.StreamIndex < 0)
      continue; // it's ERROR
    
    const CStreamInfo &si = DataStreams[item.StreamIndex];
    if (si.Resource.UnpackSize >= (1 << 16))
      continue; // reparse data can not be larger than 64 KB

    IntVector_SetMinusOne_IfNeed(streamToReparse, DataStreams.Size());
    IntVector_SetMinusOne_IfNeed(ItemToReparse, Items.Size());
    
    const unsigned offset = 0x58; // we don't know about Reparse field for OLD WIM format
    UInt32 tag = Get32(metadata + offset);
    int reparseIndex = streamToReparse[item.StreamIndex];
    CByteBuffer buf;

    if (openCallback && (i & 0xFFFF) == 0)
    {
      UInt64 numFiles = Items.Size();
      RINOK(openCallback->SetCompleted(&numFiles, NULL));
    }

    if (reparseIndex >= 0)
    {
      const CByteBuffer &reparse = ReparseItems[reparseIndex];
      if (tag == Get32(reparse))
      {
        ItemToReparse[i] = reparseIndex;
        continue;
      }
      buf = reparse;
      // we support that strange and unusual situation with different tags and same reparse data.
    }
    else
    {
      /*
      if (si.PartNumber >= volumes.Size())
        continue;
      */
      const CVolume &vol = volumes[si.PartNumber];
      /*
      if (!vol.Stream)
        continue;
      */
      
      Byte digest[kHashSize];
      HRESULT res = UnpackData(vol.Stream, si.Resource, vol.Header.IsLzxMode(), buf, digest);

      if (res == S_FALSE)
        continue;

      RINOK(res);
      
      if (memcmp(digest, si.Hash, kHashSize) != 0
        // && !(h.IsOldVersion() && IsEmptySha(si.Hash))
        )
      {
        // setErrorStatus;
        continue;
      }
    }
    
    CByteBuffer &reparse = ReparseItems.AddNew();
    reparse.Alloc(8 + buf.Size());
    Byte *dest = (Byte *)reparse;
    SetUi32(dest, tag);
    SetUi32(dest + 4, (UInt32)buf.Size());
    if (buf.Size() != 0)
      memcpy(dest + 8, buf, buf.Size());
    ItemToReparse[i] = ReparseItems.Size() - 1;
  }

  return S_OK;
}



static bool ParseNumber64(const AString &s, UInt64 &res)
{
  const char *end;
  if (s.IsPrefixedBy("0x"))
  {
    if (s.Len() == 2)
      return false;
    res = ConvertHexStringToUInt64(s.Ptr(2), &end);
  }
  else
  {
    if (s.IsEmpty())
      return false;
    res = ConvertStringToUInt64(s, &end);
  }
  return *end == 0;
}

static bool ParseNumber32(const AString &s, UInt32 &res)
{
  UInt64 res64;
  if (!ParseNumber64(s, res64) || res64 >= ((UInt64)1 << 32))
    return false;
  res = (UInt32)res64;
  return true;
}

static bool ParseTime(const CXmlItem &item, FILETIME &ft, const char *tag)
{
  int index = item.FindSubTag(tag);
  if (index >= 0)
  {
    const CXmlItem &timeItem = item.SubItems[index];
    UInt32 low = 0, high = 0;
    if (ParseNumber32(timeItem.GetSubStringForTag("LOWPART"), low) &&
        ParseNumber32(timeItem.GetSubStringForTag("HIGHPART"), high))
    {
      ft.dwLowDateTime = low;
      ft.dwHighDateTime = high;
      return true;
    }
  }
  return false;
}

void CImageInfo::Parse(const CXmlItem &item)
{
  CTimeDefined = ParseTime(item, CTime, "CREATIONTIME");
  MTimeDefined = ParseTime(item, MTime, "LASTMODIFICATIONTIME");
  NameDefined = ConvertUTF8ToUnicode(item.GetSubStringForTag("NAME"), Name);

  ParseNumber64(item.GetSubStringForTag("DIRCOUNT"), DirCount);
  ParseNumber64(item.GetSubStringForTag("FILECOUNT"), FileCount);
  IndexDefined = ParseNumber32(item.GetPropVal("INDEX"), Index);
}

void CWimXml::ToUnicode(UString &s)
{
  size_t size = Data.Size();
  if (size < 2 || (size & 1) != 0 || size > (1 << 24))
    return;
  const Byte *p = Data;
  if (Get16(p) != 0xFEFF)
    return;
  wchar_t *chars = s.GetBuf((unsigned)(size / 2));
  for (size_t i = 2; i < size; i += 2)
  {
    wchar_t c = Get16(p + i);
    if (c == 0)
      break;
    *chars++ = c;
  }
  *chars = 0;
  s.ReleaseBuf_SetLen((unsigned)(chars - (const wchar_t *)s));
}

bool CWimXml::Parse()
{
  AString utf;
  {
    UString s;
    ToUnicode(s);
    // if (!ConvertUnicodeToUTF8(s, utf)) return false;
    ConvertUnicodeToUTF8(s, utf);
  }

  if (!Xml.Parse(utf))
    return false;
  if (Xml.Root.Name != "WIM")
    return false;

  FOR_VECTOR (i, Xml.Root.SubItems)
  {
    const CXmlItem &item = Xml.Root.SubItems[i];
    if (item.IsTagged("IMAGE"))
    {
      CImageInfo imageInfo;
      imageInfo.Parse(item);
      if (!imageInfo.IndexDefined || imageInfo.Index != (UInt32)Images.Size() + 1)
        return false;
      imageInfo.ItemIndexInXml = i;
      Images.Add(imageInfo);
    }
  }
  return true;
}

}}
