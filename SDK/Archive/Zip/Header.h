// Archive/Zip/Header.h

#pragma once

#ifndef __ARCHIVE_ZIP_HEADER_H
#define __ARCHIVE_ZIP_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NZip {

#pragma pack( push, PragmaZipHeaders)
#pragma pack( push, 1)


namespace NSignature
{
  extern UINT32 kLocalFileHeader;
  extern UINT32 kDataDescriptor;
  extern UINT32 kCentralFileHeader;
  extern UINT32 kEndOfCentralDir;
  
  static const UINT32 kMarkerSize = 4;
}

struct CEndOfCentralDirectoryRecord
{
  UINT16 ThisDiskNumber;
  UINT16 StartCentralDirectoryDiskNumber;
  UINT16 NumEntriesInCentaralDirectoryOnThisDisk;
  UINT16 NumEntriesInCentaralDirectory;
  UINT32 CentralDirectorySize;
  UINT32 CentralDirectoryStartOffset;
  UINT16 CommentSize;
};

struct CEndOfCentralDirectoryRecordFull
{
  UINT32 Signature;
  CEndOfCentralDirectoryRecord Header;
};

namespace NFileHeader
{
  struct CVersion
  {
    BYTE Version;
    BYTE HostOS;
  };
  
  namespace NCompressionMethod
  {
    enum EType
    { 
      kStored = 0,
      kShrunk = 1,
      kReduced1 = 2,
      kReduced2 = 3,
      kReduced3 = 4,
      kReduced4 = 5,
      kImploded = 6,
      kReservedTokenizing = 7, // reserved for tokenizing
      kDeflated = 8, 
      kDeflated64 = 9,
      kPKImploding = 10,
      
      kBZip2 = 12
    };
    const kNumCompressionMethods = 11;
    const BYTE kMadeByProgramVersion = 20;
    
    const kDeflateExtractVersion = 20;
    const kStoreExtractVersion = 10;
    
    const BYTE kSupportedVersion   = 20;
  }

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
  
  namespace NHostOS
  {
    enum EEnum
    {
      kFAT      = 0,  // filesystem used by MS-DOS, OS/2, Win32 
        // pkzip 2.50 (FAT / VFAT / FAT32 file systems)
        kAMIGA    = 1,
        kVMS      = 2,  // VAX/VMS
        kUnix     = 3,
        kVM_CMS   = 4,
        kAtari    = 5,  // what if it's a minix filesystem? [cjh]
        kHPFS     = 6,  // filesystem used by OS/2 (and NT 3.x)
        kMac      = 7,
        kZ_System = 8,
        kCPM      = 9,
        kTOPS20   = 10, // pkzip 2.50 NTFS 
        kNTFS     = 11, // filesystem used by Windows NT 
        kQDOS     = 12, // SMS/QDOS
        kAcorn    = 13, // Archimedes Acorn RISC OS
        kVFAT     = 14, // filesystem used by Windows 95, NT
        kMVS      = 15,
        kBeOS     = 16, // hybrid POSIX/database filesystem
        // BeBOX or PowerMac 
        kTandem   = 17,
        kTHEOS    = 18
    };
    // const kNumHostSystems = 19;
  }
  namespace NUnixAttribute
  {
    const UINT32 kIFMT   =   0170000;     /* Unix file type mask */
    
    const UINT32 kIFDIR  =   0040000;     /* Unix directory */
    const UINT32 kIFREG  =   0100000;     /* Unix regular file */
    const UINT32 kIFSOCK =   0140000;     /* Unix socket (BSD, not SysV or Amiga) */
    const UINT32 kIFLNK  =   0120000;     /* Unix symbolic link (not SysV, Amiga) */
    const UINT32 kIFBLK  =   0060000;     /* Unix block special       (not Amiga) */
    const UINT32 kIFCHR  =   0020000;     /* Unix character special   (not Amiga) */
    const UINT32 kIFIFO  =   0010000;     /* Unix fifo    (BCC, not MSC or Amiga) */
    
    const UINT32 kISUID  =   04000;       /* Unix set user id on execution */
    const UINT32 kISGID  =   02000;       /* Unix set group id on execution */
    const UINT32 kISVTX  =   01000;       /* Unix directory permissions control */
    const UINT32 kENFMT  =   kISGID;   /* Unix record locking enforcement flag */
    const UINT32 kIRWXU  =   00700;       /* Unix read, write, execute: owner */
    const UINT32 kIRUSR  =   00400;       /* Unix read permission: owner */
    const UINT32 kIWUSR  =   00200;       /* Unix write permission: owner */
    const UINT32 kIXUSR  =   00100;       /* Unix execute permission: owner */
    const UINT32 kIRWXG  =   00070;       /* Unix read, write, execute: group */
    const UINT32 kIRGRP  =   00040;       /* Unix read permission: group */
    const UINT32 kIWGRP  =   00020;       /* Unix write permission: group */
    const UINT32 kIXGRP  =   00010;       /* Unix execute permission: group */
    const UINT32 kIRWXO  =   00007;       /* Unix read, write, execute: other */
    const UINT32 kIROTH  =   00004;       /* Unix read permission: other */
    const UINT32 kIWOTH  =   00002;       /* Unix write permission: other */
    const UINT32 kIXOTH  =   00001;       /* Unix execute permission: other */
  }
  
  namespace NAmigaAttribute
  {
    const UINT32 kIFMT     = 06000;       /* Amiga file type mask */
    const UINT32 kIFDIR    = 04000;       /* Amiga directory */
    const UINT32 kIFREG    = 02000;       /* Amiga regular file */
    const UINT32 kIHIDDEN  = 00200;       /* to be supported in AmigaDOS 3.x */
    const UINT32 kISCRIPT  = 00100;       /* executable script (text command file) */
    const UINT32 kIPURE    = 00040;       /* allow loading into resident memory */
    const UINT32 kIARCHIVE = 00020;       /* not modified since bit was last set */
    const UINT32 kIREAD    = 00010;       /* can be opened for reading */
    const UINT32 kIWRITE   = 00004;       /* can be opened for writing */
    const UINT32 kIEXECUTE = 00002;       /* executable image, a loadable runfile */
    const UINT32 kIDELETE  = 00001;      /* can be deleted */
  }
}

#pragma pack(pop)
#pragma pack(pop, PragmaZipHeaders)

}}

#endif
