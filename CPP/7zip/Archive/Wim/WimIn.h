// Archive/WimIn.h

#ifndef __ARCHIVE_WIM_IN_H
#define __ARCHIVE_WIM_IN_H

#include "Common/Buffer.h"
#include "Common/MyString.h"

#include "../../Compress/CopyCoder.h"
#include "../../Compress/LzxDecoder.h"

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

const int kNumHuffmanBits = 16;
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
  const Byte Compressed = 4;
  const Byte kMetadata = 2;
}

struct CResource
{
  UInt64 PackSize;
  UInt64 Offset;
  UInt64 UnpackSize;
  Byte Flags;

  void Parse(const Byte *p);
  bool IsCompressed() const { return (Flags & NResourceFlags::Compressed) != 0; }
  bool IsMetadata() const { return (Flags & NResourceFlags::kMetadata) != 0; }
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

struct CHeader
{
  UInt32 Flags;
  UInt32 Version;
  // UInt32 ChunkSize;
  UInt16 PartNumber;
  UInt16 NumParts;
  UInt32 NumImages;
  Byte Guid[16];
  
  CResource OffsetResource;
  CResource XmlResource;
  CResource MetadataResource;
  /*
  CResource IntegrityResource;
  UInt32 BootIndex;
  */

  HRESULT Parse(const Byte *p);
  bool IsCompressed() const { return (Flags & NHeaderFlags::kCompression) != 0; }
  bool IsSupported() const { return (!IsCompressed() || (Flags & NHeaderFlags::kLZX) != 0 || (Flags & NHeaderFlags::kXPRESS) != 0 ) ; }
  bool IsLzxMode() const { return (Flags & NHeaderFlags::kLZX) != 0; }
  bool IsSpanned() const { return (!IsCompressed() || (Flags & NHeaderFlags::kSpanned) != 0); }

  bool IsNewVersion()const { return (Version > 0x010C00); }

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
  BYTE Hash[kHashSize];
};

struct CItem
{
  UString Name;
  UInt32 Attrib;
  // UInt32 SecurityId;
  BYTE Hash[kHashSize];
  FILETIME CTime;
  FILETIME ATime;
  FILETIME MTime;
  // UInt32 ReparseTag;
  // UInt64 HardLink;
  // UInt16 NumStreams;
  // UInt16 ShortNameLen;
  int StreamIndex;
  bool HasMetadata;
  CItem(): HasMetadata(true), StreamIndex(-1) {}
  bool isDir() const { return HasMetadata && ((Attrib & 0x10) != 0); }
  bool HasStream() const
  {
    for (int i = 0; i < kHashSize; i++)
      if (Hash[i] != 0)
        return true;
    return false;
  }
};

struct CDatabase
{
  CRecordVector<CStreamInfo> Streams;
  CObjectVector<CItem> Items;

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
  }
};

HRESULT ReadHeader(IInStream *inStream, CHeader &header);
HRESULT OpenArchive(IInStream *inStream, const CHeader &header, CByteBuffer &xml, CDatabase &database);
HRESULT SortDatabase(CDatabase &database);

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
