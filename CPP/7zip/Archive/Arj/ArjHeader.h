// Archive/Arj/Header.h

#ifndef __ARCHIVE_ARJ_HEADER_H
#define __ARCHIVE_ARJ_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NArj {

const int kMaxBlockSize = 2600;

namespace NSignature
{
  const Byte kSig0 = 0x60;
  const Byte kSig1 = 0xEA;
}

/*
struct CArchiveHeader
{
  // UInt16 BasicHeaderSize;
  Byte FirstHeaderSize;
  Byte Version;
  Byte ExtractVersion;
  Byte HostOS;
  Byte Flags;
  Byte SecuryVersion;
  Byte FileType;
  Byte Reserved;
  UInt32 CreatedTime;
  UInt32 ModifiedTime;
  UInt32 ArchiveSize;
  UInt32 SecurityEnvelopeFilePosition;
  UInt16 FilespecPositionInFilename;
  UInt16 LengthOfSecurityEnvelopeSata;
  Byte EncryptionVersion;
  Byte LastChapter;
};
*/

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
      kNoData = 9
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
      kChapterLabel = 5
    };
  }
  namespace NFlags
  {
    const Byte kGarbled = 1;
    const Byte kVolume = 4;
    const Byte kExtFile = 8;
    const Byte kPathSym = 0x10;
    const Byte kBackup= 0x20;
  }

  /*
  struct CHeader
  {
    Byte FirstHeaderSize;
    Byte Version;
    Byte ExtractVersion;
    Byte HostOS;
    Byte Flags;
    Byte Method;
    Byte FileType;
    Byte Reserved;
    UInt32 ModifiedTime;
    UInt32 PackSize;
    UInt32 Size;
    UInt32 FileCRC;
    UInt16 FilespecPositionInFilename;
    UInt16 FileAccessMode;
    Byte FirstChapter;
    Byte LastChapter;
  };
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
  }
}

}}

#endif
