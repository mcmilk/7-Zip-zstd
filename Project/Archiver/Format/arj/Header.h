// Archive/arj/Header.h

#pragma once

#ifndef __ARCHIVE_arj_HEADER_H
#define __ARCHIVE_arj_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace Narj {

#pragma pack( push, Pragma_arj_Headers)
#pragma pack( push, 1)

const kMaxBlockSize = 2600;

namespace NSignature
{
  const BYTE kSig0 = 0x60;
  const BYTE kSig1 = 0xEA;
}

struct CArchiveHeader
{
  // UINT16 BasicHeaderSize;
  BYTE FirstHeaderSize;
  BYTE Version;
  BYTE ExtractVersion;
  BYTE HostOS;
  BYTE Flags;
  BYTE SecuryVersion;
  BYTE FileType;
  BYTE Reserved;
  UINT32 CreatedTime;
  UINT32 ModifiedTime;
  UINT32 ArchiveSize;
  UINT32 SecurityEnvelopeFilePosition;
  UINT16 FilespecPositionInFilename;
  UINT16 LengthOfSecurityEnvelopeSata;
  BYTE EncryptionVersion;
  BYTE LastChapter;
};

namespace NFileHeader
{
  namespace NCompressionMethod
  {
    enum EType
    { 
      kStored = 0,
      kCompressed1a = 1,
      kCompressed1b = 2,
      kCompressed1c = 3,
      kCompressed2 = 4,
      kNoDataNoCRC = 8,
      kNoData = 9,
    };
  }
  namespace NFileType
  {
    enum EType
    { 
      kBinary = 0,
      k7BitText = 1,
      kDirectory = 3,
      kVolumeLablel = 4,
      kChapterLabel = 5,
    };
  }
  namespace NFlags
  {
    const BYTE kGarbled = 1;
    const BYTE kVolume = 4;
    const BYTE kExtFile = 8;
    const BYTE kPathSym = 0x10;
    const BYTE kBackup= 0x20;
  }

  struct CHeader
  {
    BYTE FirstHeaderSize;
    BYTE Version;
    BYTE ExtractVersion;
    BYTE HostOS;
    BYTE Flags;
    BYTE Method;
    BYTE FileType;
    BYTE Reserved;
    UINT32 ModifiedTime;
    UINT32 PackSize;
    UINT32 Size;
    UINT32 FileCRC;
    UINT16 FilespecPositionInFilename;
    UINT16 FileAccessMode;
    BYTE FirstChapter;
    BYTE LastChapter;
  };
  /*
  struct CLocalBlock
  {
    CVersion ExtractVersion;
    
    UINT16 Flags;
    UINT16 CompressionMethod;
    UINT32 Time;
    UINT32 FileCRC;
    UINT32 PackSize;
    UINT32 UnPackSize;
    UINT16 NameSize;
    UINT16 ExtraSize;
  };

  struct CDataDescriptor
  {
    UINT32 Signature;
    UINT32 FileCRC;
    UINT32 PackSize;
    UINT32 UnPackSize;
  };

  struct CLocalBlockFull
  {
    UINT32 Signature;
    CLocalBlock Header;
  };
  
  struct CBlock
  {
    CVersion MadeByVersion;
    CVersion ExtractVersion;
    UINT16 Flags;
    UINT16 CompressionMethod;
    UINT32 Time;
    UINT32 FileCRC;
    UINT32 PackSize;
    UINT32 UnPackSize;
    UINT16 NameSize;
    UINT16 ExtraSize;
    UINT16 CommentSize;
    UINT16 DiskNumberStart;
    UINT16 InternalAttributes;
    UINT32 ExternalAttributes;
    UINT32 LocalHeaderOffset;
  };
  
  struct CBlockFull
  {
    UINT32 Signature;
    CBlock Header;
  };
  
  namespace NFlags 
  {
    const kNumUsedBits = 4;
    const kUsedBitsMask = (1 << kNumUsedBits) - 1;
    
    const kEncryptedMask   = 1 << 0;
    const kDescriptorUsedMask   = 1 << 3;
    
    const kImplodeDictionarySizeMask = 1 << 1;
    const kImplodeLiteralsOnMask     = 1 << 2;
    
    const kDeflateTypeBitStart = 1;
    const kNumDeflateTypeBits = 2;
    const kNumDeflateTypes = (1 << kNumDeflateTypeBits);
    const kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
  }
  
  */
  namespace NHostOS
  {
    enum EEnum
    {
        kMSDOS    = 0,  // filesystem used by MS-DOS, OS/2, Win32 
        // pkarj 2.50 (FAT / VFAT / FAT32 file systems)
        kPRIMOS   = 1,
        kUnix     = 2,  // VAX/VMS
        kAMIGA    = 3,
        kMac      = 4,
        kOS_2     = 5,  // what if it's a minix filesystem? [cjh]
        kAPPLE_GS = 6,  // filesystem used by OS/2 (and NT 3.x)
        kAtari_ST = 7,
        kNext     = 8,
        kVAX_VMS  = 9,
        kWIN95   = 10
    };
    // const kNumHostSystems = 19;
  }
}

#pragma pack(pop)
#pragma pack(pop, Pragma_arj_Headers)

}}

#endif
