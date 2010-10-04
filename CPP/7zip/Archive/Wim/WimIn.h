// Archive/WimIn.h

#ifndef __ARCHIVE_WIM_IN_H
#define __ARCHIVE_WIM_IN_H

#include "Common/Buffer.h"
#include "Common/MyString.h"

#include "../../Compress/CopyCoder.h"
#include "../../Compress/LzxDecoder.h"

#include "../IArchive.h"

namespace NArchive {
namespace NWim {

namespace NXpress {

class CBitStream
{
  CInBuffer m_Stream;
  UInt32 m_Value;
  unsigned m_BitPos;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *s) { m_Stream.SetStream(s); }
  void ReleaseStream() { m_Stream.ReleaseStream(); }

  void Init() { m_Stream.Init(); m_BitPos = 0; }
  // UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() - m_BitPos / 8; }
  Byte DirectReadByte() { return m_Stream.ReadByte(); }

  void Normalize()
  {
    if (m_BitPos < 16)
    {
      Byte b0 = m_Stream.ReadByte();
      Byte b1 = m_Stream.ReadByte();
      m_Value = (m_Value << 8) | b1;
      m_Value = (m_Value << 8) | b0;
      m_BitPos += 16;
    }
  }

  UInt32 GetValue(unsigned numBits)
  {
    Normalize();
    return (m_Value >> (m_BitPos - numBits)) & ((1 << numBits) - 1);
  }
  
  void MovePos(unsigned numBits) { m_BitPos -= numBits; }
  
  UInt32 ReadBits(unsigned numBits)
  {
    UInt32 res = GetValue(numBits);
    m_BitPos -= numBits;
    return res;
  }
};

const unsigned kNumHuffmanBits = 16;
const UInt32 kMatchMinLen = 3;
const UInt32 kNumLenSlots = 16;
const UInt32 kNumPosSlots = 16;
const UInt32 kNumPosLenSlots = kNumPosSlots * kNumLenSlots;
const UInt32 kMainTableSize = 256 + kNumPosLenSlots;

class CDecoder
{
  CBitStream m_InBitStream;
  CLzOutWindow m_OutWindowStream;
  NCompress::NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize> m_MainDecoder;

  HRESULT CodeSpec(UInt32 size);
  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt32 outSize);
public:
  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }
  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt32 outSize);
};

}

namespace NResourceFlags
{
  const Byte kFree = 1;
  const Byte kMetadata = 2;
  const Byte Compressed = 4;
  const Byte Spanned = 4;
}

struct CResource
{
  UInt64 PackSize;
  UInt64 Offset;
  UInt64 UnpackSize;
  Byte Flags;

  void Clear()
  {
    PackSize = 0;
    Offset = 0;
    UnpackSize = 0;
    Flags = 0;
  }
  void Parse(const Byte *p);
  void WriteTo(Byte *p) const;
  bool IsFree() const { return (Flags & NResourceFlags::kFree) != 0; }
  bool IsMetadata() const { return (Flags & NResourceFlags::kMetadata) != 0; }
  bool IsCompressed() const { return (Flags & NResourceFlags::Compressed) != 0; }
  bool IsEmpty() const { return (UnpackSize == 0); }
};

namespace NHeaderFlags
{
  const UInt32 kCompression = 2;
  const UInt32 kSpanned = 8;
  const UInt32 kRpFix = 0x80;
  const UInt32 kXPRESS = 0x20000;
  const UInt32 kLZX = 0x40000;
}

const UInt32 kWimVersion = 0x010D00;
const UInt32 kHeaderSizeMax = 0xD0;
const UInt32 kSignatureSize = 8;
extern const Byte kSignature[kSignatureSize];
const unsigned kChunkSizeBits = 15;
const UInt32 kChunkSize = (1 << kChunkSizeBits);

struct CHeader
{
  UInt32 Version;
  UInt32 Flags;
  UInt32 ChunkSize;
  Byte Guid[16];
  UInt16 PartNumber;
  UInt16 NumParts;
  UInt32 NumImages;
  
  CResource OffsetResource;
  CResource XmlResource;
  CResource MetadataResource;
  CResource IntegrityResource;
  UInt32 BootIndex;

  void SetDefaultFields(bool useLZX);

  void WriteTo(Byte *p) const;
  HRESULT Parse(const Byte *p);
  bool IsCompressed() const { return (Flags & NHeaderFlags::kCompression) != 0; }
  bool IsSupported() const { return (!IsCompressed() || (Flags & NHeaderFlags::kLZX) != 0 || (Flags & NHeaderFlags::kXPRESS) != 0 ) ; }
  bool IsLzxMode() const { return (Flags & NHeaderFlags::kLZX) != 0; }
  bool IsSpanned() const { return (!IsCompressed() || (Flags & NHeaderFlags::kSpanned) != 0); }
  bool IsOldVersion() const { return (Version <= 0x010A00); }
  bool IsNewVersion() const { return (Version > 0x010C00); }

  bool AreFromOnArchive(const CHeader &h)
  {
    return (memcmp(Guid, h.Guid, sizeof(Guid)) == 0) && (h.NumParts == NumParts);
  }
};

const UInt32 kHashSize = 20;
const UInt32 kStreamInfoSize = 24 + 2 + 4 + kHashSize;

struct CStreamInfo
{
  CResource Resource;
  UInt16 PartNumber;
  UInt32 RefCount;
  UInt32 Id;
  BYTE Hash[kHashSize];

  void WriteTo(Byte *p) const;
};

const UInt32 kDirRecordSizeOld = 62;
const UInt32 kDirRecordSize = 102;

struct CItem
{
  UString Name;
  UString ShortName;
  UInt32 Attrib;
  // UInt32 SecurityId;
  BYTE Hash[kHashSize];
  UInt32 Id;
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  // UInt32 ReparseTag;
  // UInt64 HardLink;
  // UInt16 NumStreams;
  int StreamIndex;
  int Parent;
  unsigned Order;
  bool HasMetadata;
  CItem(): HasMetadata(true), StreamIndex(-1), Id(0) {}
  bool IsDir() const { return HasMetadata && ((Attrib & 0x10) != 0); }
  bool HasStream() const
  {
    for (unsigned i = 0; i < kHashSize; i++)
      if (Hash[i] != 0)
        return true;
    return Id != 0;
  }
};

class CDatabase
{
  const Byte *DirData;
  size_t DirSize;
  size_t DirProcessed;
  size_t DirStartOffset;
  int Order;
  IArchiveOpenCallback *OpenCallback;
  
  HRESULT ParseDirItem(size_t pos, int parent);
  HRESULT ParseImageDirs(const CByteBuffer &buf, int parent);

public:
  CRecordVector<CStreamInfo> Streams;
  CObjectVector<CItem> Items;
  CIntVector SortedItems;
  int NumImages;
  bool SkipRoot;
  bool ShowImageNumber;

  bool IsOldVersion;

  UInt64 GetUnpackSize() const
  {
    UInt64 res = 0;
    for (int i = 0; i < Streams.Size(); i++)
      res += Streams[i].Resource.UnpackSize;
    return res;
  }

  UInt64 GetPackSize() const
  {
    UInt64 res = 0;
    for (int i = 0; i < Streams.Size(); i++)
      res += Streams[i].Resource.PackSize;
    return res;
  }

  void Clear()
  {
    Streams.Clear();
    Items.Clear();
    SortedItems.Clear();
    NumImages = 0;

    SkipRoot = true;
    ShowImageNumber = true;
    IsOldVersion = false;
  }

  UString GetItemPath(int index) const;

  HRESULT Open(IInStream *inStream, const CHeader &h, CByteBuffer &xml, IArchiveOpenCallback *openCallback);

  void DetectPathMode()
  {
    ShowImageNumber = (NumImages != 1);
  }

  HRESULT Sort(bool skipRootDir);
};

HRESULT ReadHeader(IInStream *inStream, CHeader &header);

class CUnpacker
{
  NCompress::CCopyCoder *copyCoderSpec;
  CMyComPtr<ICompressCoder> copyCoder;

  NCompress::NLzx::CDecoder *lzxDecoderSpec;
  CMyComPtr<ICompressCoder> lzxDecoder;

  NXpress::CDecoder xpressDecoder;

  CByteBuffer sizesBuf;
  HRESULT Unpack(IInStream *inStream, const CResource &res, bool lzxMode,
      ISequentialOutStream *outStream, ICompressProgressInfo *progress);
public:
  HRESULT Unpack(IInStream *inStream, const CResource &res, bool lzxMode,
      ISequentialOutStream *outStream, ICompressProgressInfo *progress, Byte *digest);
};

}}
  
#endif
