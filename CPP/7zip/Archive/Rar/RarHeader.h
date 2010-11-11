// Archive/RarHeader.h

#ifndef __ARCHIVE_RAR_HEADER_H
#define __ARCHIVE_RAR_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NRar {
namespace NHeader {

const int kMarkerSize = 7;
extern Byte kMarker[kMarkerSize];
  
const int kArchiveSolid = 0x1;

namespace NBlockType
{
  enum EBlockType
  {
    kMarker = 0x72,
    kArchiveHeader,
    kFileHeader,
    kCommentHeader,
    kOldAuthenticity,
    kOldSubBlock,
    kRecoveryRecord,
    kAuthenticity,
    kSubBlock,
    kEndOfArchive
  };
}

namespace NArchive
{
  const UInt16 kVolume  = 1;
  const UInt16 kComment = 2;
  const UInt16 kLock    = 4;
  const UInt16 kSolid   = 8;
  const UInt16 kNewVolName = 0x10; // ('volname.partN.rar')
  const UInt16 kAuthenticity  = 0x20;
  const UInt16 kRecovery = 0x40;
  const UInt16 kBlockEncryption  = 0x80;
  const UInt16 kFirstVolume = 0x100; // (set only by RAR 3.0 and later)
  const UInt16 kEncryptVer = 0x200; // RAR 3.6 there is EncryptVer Byte in End of MainHeader

  const int kHeaderSizeMin = 7;
  
  const int kArchiveHeaderSize = 13;

  const int kBlockHeadersAreEncrypted = 0x80;

}

namespace NFile
{
  const int kSplitBefore = 1 << 0;
  const int kSplitAfter  = 1 << 1;
  const int kEncrypted   = 1 << 2;
  const int kComment     = 1 << 3;
  const int kSolid       = 1 << 4;
  
  const int kDictBitStart     = 5;
  const int kNumDictBits  = 3;
  const int kDictMask         = (1 << kNumDictBits) - 1;
  const int kDictDirectoryValue  = 0x7;
  
  const int kSize64Bits    = 1 << 8;
  const int kUnicodeName   = 1 << 9;
  const int kSalt          = 1 << 10;
  const int kOldVersion    = 1 << 11;
  const int kExtTime       = 1 << 12;
  // const int kExtFlags      = 1 << 13;
  // const int kSkipIfUnknown = 1 << 14;

  const int kLongBlock    = 1 << 15;
  
  /*
  struct CBlock
  {
    // UInt16 HeadCRC;
    // Byte Type;
    // UInt16 Flags;
    // UInt16 HeadSize;
    UInt32 PackSize;
    UInt32 UnPackSize;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
  };
  */

  /*
  struct CBlock32
  {
    UInt16 HeadCRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    UInt32 PackSize;
    UInt32 UnPackSize;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
    UInt16 GetRealCRC(const void *aName, UInt32 aNameSize,
        bool anExtraDataDefined = false, Byte *anExtraData = 0) const;
  };
  struct CBlock64
  {
    UInt16 HeadCRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    UInt32 PackSizeLow;
    UInt32 UnPackSizeLow;
    Byte HostOS;
    UInt32 FileCRC;
    UInt32 Time;
    Byte UnPackVersion;
    Byte Method;
    UInt16 NameSize;
    UInt32 Attributes;
    UInt32 PackSizeHigh;
    UInt32 UnPackSizeHigh;
    UInt16 GetRealCRC(const void *aName, UInt32 aNameSize) const;
  };
  */
  
  const int kLabelFileAttribute            = 0x08;
  const int kWinFileDirectoryAttributeMask = 0x10;
  
  enum CHostOS
  {
    kHostMSDOS = 0,
      kHostOS2   = 1,
      kHostWin32 = 2,
      kHostUnix  = 3,
      kHostMacOS = 4,
      kHostBeOS = 5
  };
}

namespace NBlock
{
  const UInt16 kLongBlock = 1 << 15;
  struct CBlock
  {
    UInt16 CRC;
    Byte Type;
    UInt16 Flags;
    UInt16 HeadSize;
    //  UInt32 DataSize;
  };
}

/*
struct CSubBlock
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt32 DataSize;
  UInt16 SubType;
  Byte Level; // Reserved : Must be 0
};

struct CCommentBlock
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt16 UnpSize;
  Byte UnpVer;
  Byte Method;
  UInt16 CommCRC;
};


struct CProtectHeader
{
  UInt16 HeadCRC;
  Byte HeadType;
  UInt16 Flags;
  UInt16 HeadSize;
  UInt32 DataSize;
  Byte Version;
  UInt16 RecSectors;
  UInt32 TotalBlocks;
  Byte Mark[8];
};
*/

}}}

#endif
