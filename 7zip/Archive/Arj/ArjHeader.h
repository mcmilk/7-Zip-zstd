// Archive/Arj/Header.h

#pragma once

#ifndef __ARCHIVE_ARJ_HEADER_H
#define __ARCHIVE_ARJ_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NArj {

const int kMaxBlockSize = 2600;

namespace NSignature
{
  const BYTE kSig0 = 0x60;
  const BYTE kSig1 = 0xEA;
}

#pragma pack( push, Pragma_Arj_Headers)
#pragma pack( push, 1)

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
  }
}

#pragma pack(pop)
#pragma pack(pop, Pragma_Arj_Headers)

}}

#endif
