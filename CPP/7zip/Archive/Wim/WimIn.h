// Archive/WimIn.h

#ifndef __ARCHIVE_WIM_IN_H
#define __ARCHIVE_WIM_IN_H

#include "../../../Common/MyBuffer.h"
#include "../../../Common/MyXml.h"

#include "../../../Windows/PropVariant.h"

#include "../../Compress/CopyCoder.h"
#include "../../Compress/LzxDecoder.h"

#include "../IArchive.h"

namespace NArchive {
namespace NWim {

const UInt32 kDirRecordSizeOld = 62;
const UInt32 kDirRecordSize = 102;

/*
  There is error in WIM specification about dwReparseTag, dwReparseReserved and liHardLink fields.

  Correct DIRENTRY structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt32  Attrib;
     C    UInt32  SecurityId;
    
    10    UInt64  SubdirOffset; // = 0 for files

    18    UInt64  unused1; // = 0?
    20    UInt64  unused2; // = 0?
    
    28    UInt64  CTime;
    30    UInt64  ATime;
    38    UInt64  MTime;
    
    40    Byte    Sha1[20];
    
    54    UInt32  Unknown1; // is it 0 always?

       
    union
    {
    58    UInt64  NtNodeId;
        {
    58    UInt32  ReparseTag;
    5C    UInt32  ReparseFlags; // is it 0 always? Check with new imagex.
        }
    }

    60    UInt16  Streams;
    
    62    UInt16  ShortNameLen;
    64    UInt16  FileNameLen;
    
    66    UInt16  Name[];
          UInt16  ShortName[];
  }

  // DIRENTRY for WIM_VERSION <= 1.10
  DIRENTRY_OLD structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt32  Attrib;
     C    UInt32  SecurityId;

    union
    {
    10    UInt64  SubdirOffset; //

    10    UInt32  OldWimFileId; // used for files in old WIMs
    14    UInt32  OldWimFileId_Reserved; // = 0
    }

    18    UInt64  CTime;
    20    UInt64  ATime;
    28    UInt64  MTime;
    
    30    UInt64  Unknown; // NtNodeId ?

    38    UInt16  Streams;
    3A    UInt16  ShortNameLen;
    3C    UInt16  FileNameLen;
    3E    UInt16  FileName[];
          UInt16  ShortName[];
  }

  ALT_STREAM structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt64  Unused;
    10    Byte    Sha1[20];
    24    UInt16  FileNameLen;
    26    UInt16  FileName[];
  }

  ALT_STREAM_OLD structure:
  {
    hex offset
     0    UInt64  Len;
     8    UInt64  StreamId; // 32-bit value
    10    UInt16  FileNameLen;
    12    UInt16  FileName[];
  }

  If item is file (not Directory) and there are alternative streams,
  there is additional ALT_STREAM item of main "unnamed" stream in Streams array.

*/

namespace NXpress {

class CBitStream
{
  CInBuffer m_Stream;
  UInt32 m_Value;
  unsigned m_BitPos;
public:
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *s) { m_Stream.SetStream(s); }

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
  HRESULT Flush() { return m_OutWindowStream.Flush(); }
  HRESULT Code(ISequentialInStream *inStream, ISequentialOutStream *outStream, UInt32 outSize);
};

}

namespace NResourceFlags
{
  const Byte kFree = 1;
  const Byte kMetadata = 2;
  const Byte Compressed = 4;
  // const Byte Spanned = 4;
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
  UInt64 GetEndLimit() const { return Offset + PackSize; }
  void Parse(const Byte *p);
  void ParseAndUpdatePhySize(const Byte *p, UInt64 &phySize)
  {
    Parse(p);
    UInt64 v = GetEndLimit();
    if (phySize < v)
      phySize = v;
  }

  void WriteTo(Byte *p) const;
  bool IsFree() const { return (Flags & NResourceFlags::kFree) != 0; }
  bool IsMetadata() const { return (Flags & NResourceFlags::kMetadata) != 0; }
  bool IsCompressed() const { return (Flags & NResourceFlags::Compressed) != 0; }
  bool IsEmpty() const { return (UnpackSize == 0); }
};

namespace NHeaderFlags
{
  const UInt32 kCompression  = 2;
  const UInt32 kReadOnly     = 4;
  const UInt32 kSpanned      = 8;
  const UInt32 kResourceOnly = 0x10;
  const UInt32 kMetadataOnly = 0x20;
  const UInt32 kWriteInProgress = 0x40;
  const UInt32 kReparsePointFixup = 0x80;
  const UInt32 kXPRESS       = 0x20000;
  const UInt32 kLZX          = 0x40000;
}

const UInt32 kWimVersion = 0x010D00;

const unsigned kHeaderSizeMax = 0xD0;
const unsigned kSignatureSize = 8;
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
  HRESULT Parse(const Byte *p, UInt64 &phySize);
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

const unsigned kHashSize = 20;
const unsigned kStreamInfoSize = 24 + 2 + 4 + kHashSize;

struct CStreamInfo
{
  CResource Resource;
  UInt16 PartNumber;      // for NEW WIM format, we set it to 1 for OLD WIM format
  UInt32 RefCount;
  UInt32 Id;              // for OLD WIM format
  Byte Hash[kHashSize];

  void WriteTo(Byte *p) const;
};

struct CItem
{
  size_t Offset;
  int IndexInSorted;
  int StreamIndex;
  int Parent;
  int ImageIndex; // -1 means that file is unreferenced in Images (deleted item?)
  bool IsDir;
  bool IsAltStream;

  bool HasMetadata() const { return ImageIndex >= 0; }

  CItem():
    IndexInSorted(-1),
    StreamIndex(-1),
    Parent(-1),
    IsDir(false),
    IsAltStream(false)
    {}
};

struct CImage
{
  CByteBuffer Meta;
  CRecordVector<UInt32> SecurOffsets;
  unsigned StartItem;
  unsigned NumItems;
  unsigned NumEmptyRootItems;
  int VirtualRootIndex; // index in CDatabase::VirtualRoots[]
  UString RootName;
  CByteBuffer RootNameBuf;

  CImage(): VirtualRootIndex(-1) {}
};

struct CImageInfo
{
  bool CTimeDefined;
  bool MTimeDefined;
  bool NameDefined;
  bool IndexDefined;
  
  FILETIME CTime;
  FILETIME MTime;
  UString Name;

  UInt64 DirCount;
  UInt64 FileCount;
  UInt32 Index;

  int ItemIndexInXml;

  UInt64 GetTotalFilesAndDirs() const { return DirCount + FileCount; }
  
  CImageInfo(): CTimeDefined(false), MTimeDefined(false), NameDefined(false),
      IndexDefined(false), ItemIndexInXml(-1) {}
  void Parse(const CXmlItem &item);
};

struct CWimXml
{
  CByteBuffer Data;
  CXml Xml;

  UInt16 VolIndex;
  CObjectVector<CImageInfo> Images;

  UString FileName;

  UInt64 GetTotalFilesAndDirs() const
  {
    UInt64 sum = 0;
    FOR_VECTOR (i, Images)
      sum += Images[i].GetTotalFilesAndDirs();
    return sum;
  }

  void ToUnicode(UString &s);
  bool Parse();
};

struct CVolume
{
  CHeader Header;
  CMyComPtr<IInStream> Stream;
};

class CDatabase
{
  Byte *DirData;
  size_t DirSize;
  size_t DirProcessed;
  size_t DirStartOffset;
  IArchiveOpenCallback *OpenCallback;

  HRESULT ParseDirItem(size_t pos, int parent);
  HRESULT ParseImageDirs(CByteBuffer &buf, int parent);

public:
  CRecordVector<CStreamInfo> DataStreams;
  

  CRecordVector<CStreamInfo> MetaStreams;
  
  CRecordVector<CItem> Items;
  CObjectVector<CByteBuffer> ReparseItems;
  CIntVector ItemToReparse; // from index_in_Items to index_in_ReparseItems
                            // -1 means no reparse;
  
  CObjectVector<CImage> Images;
  
  bool IsOldVersion;
  bool ThereAreDeletedStreams;
  bool ThereAreAltStreams;
  bool RefCountError;
  
  // User Items can contain all images or just one image from all.
  CUIntVector SortedItems;
  int IndexOfUserImage;    // -1 : if more than one images was filled to Sorted Items
  
  unsigned NumExcludededItems;
  int ExludedItem;          // -1 : if there are no exclude items
  CUIntVector VirtualRoots; // we use them for old 1.10 WIM archives

  bool ThereIsError() const { return RefCountError; }

  unsigned GetNumUserItemsInImage(unsigned imageIndex) const
  {
    if (IndexOfUserImage >= 0 && imageIndex != (unsigned)IndexOfUserImage)
      return 0;
    if (imageIndex >= Images.Size())
      return 0;
    return Images[imageIndex].NumItems - NumExcludededItems;
  }

  bool ItemHasStream(const CItem &item) const;

  UInt64 GetUnpackSize() const
  {
    UInt64 res = 0;
    FOR_VECTOR (i, DataStreams)
      res += DataStreams[i].Resource.UnpackSize;
    return res;
  }

  UInt64 GetPackSize() const
  {
    UInt64 res = 0;
    FOR_VECTOR (i, DataStreams)
      res += DataStreams[i].Resource.PackSize;
    return res;
  }

  void Clear()
  {
    DataStreams.Clear();

    MetaStreams.Clear();
    
    Items.Clear();
    ReparseItems.Clear();
    ItemToReparse.Clear();

    SortedItems.Clear();
    
    Images.Clear();
    VirtualRoots.Clear();

    IsOldVersion = false;
    ThereAreDeletedStreams = false;
    ThereAreAltStreams = false;
    RefCountError = false;
  }

  CDatabase(): RefCountError(false) {}

  void GetShortName(unsigned index, NWindows::NCOM::CPropVariant &res) const;
  void GetItemName(unsigned index1, NWindows::NCOM::CPropVariant &res) const;
  void GetItemPath(unsigned index, bool showImageNumber, NWindows::NCOM::CPropVariant &res) const;

  HRESULT OpenXml(IInStream *inStream, const CHeader &h, CByteBuffer &xml);
  HRESULT Open(IInStream *inStream, const CHeader &h, unsigned numItemsReserve, IArchiveOpenCallback *openCallback);
  HRESULT FillAndCheck();

  /*
    imageIndex showImageNumber NumImages
         *        true           *       Show Image_Number
        -1           *          >1       Show Image_Number
        -1        false          1       Don't show Image_Number
         N        false          *       Don't show Image_Number
  */
  HRESULT GenerateSortedItems(int imageIndex, bool showImageNumber);

  HRESULT ExtractReparseStreams(const CObjectVector<CVolume> &volumes, IArchiveOpenCallback *openCallback);
};

HRESULT ReadHeader(IInStream *inStream, CHeader &header, UInt64 &phySize);

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
