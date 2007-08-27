// Archive/WimIn.h

#ifndef __ARCHIVE_WIM_IN_H
#define __ARCHIVE_WIM_IN_H

#include "Common/MyString.h"
#include "Common/Buffer.h"

#include "../../Compress/Lzx/LzxDecoder.h"
#include "../../Compress/Copy/CopyCoder.h"

namespace NArchive {
namespace NWim {

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
  bool IsCompressed() const { return (Flags & NHeaderFlags::kCompression) != 0; }
  bool IsSupported() const { return (!IsCompressed() || (Flags & NHeaderFlags::kLZX) != 0); }
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
  UInt32 Attributes;
  // UInt32 SecurityId;
  BYTE Hash[kHashSize];
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  // UInt32 ReparseTag;
  // UInt64 HardLink;
  // UInt16 NumStreams;
  // UInt16 ShortNameLen;
  int StreamIndex;
  bool HasMetadata;
  CItem(): HasMetadata(true), StreamIndex(-1) {}
  bool IsDirectory() const { return HasMetadata && ((Attributes & 0x10) != 0); }
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

  CByteBuffer sizesBuf;
  HRESULT Unpack(IInStream *inStream, const CResource &res, 
      ISequentialOutStream *outStream, ICompressProgressInfo *progress);
public:
  HRESULT Unpack(IInStream *inStream, const CResource &res, 
      ISequentialOutStream *outStream, ICompressProgressInfo *progress, Byte *digest);
};

}}
  
#endif
