// Archive/Zip/Header.h

#ifndef __ARCHIVE_ZIP_HEADER_H
#define __ARCHIVE_ZIP_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NZip {

namespace NSignature
{
  extern UInt32 kLocalFileHeader;
  extern UInt32 kDataDescriptor;
  extern UInt32 kCentralFileHeader;
  extern UInt32 kEndOfCentralDir;
  extern UInt32 kZip64EndOfCentralDir;
  extern UInt32 kZip64EndOfCentralDirLocator;
  
  static const UInt32 kMarkerSize = 4;
}

const UInt32 kEcdSize = 22;
const UInt32 kZip64EcdSize = 44;
const UInt32 kZip64EcdLocatorSize = 20;
/*
struct CEndOfCentralDirectoryRecord
{
  UInt16 ThisDiskNumber;
  UInt16 StartCentralDirectoryDiskNumber;
  UInt16 NumEntriesInCentaralDirectoryOnThisDisk;
  UInt16 NumEntriesInCentaralDirectory;
  UInt32 CentralDirectorySize;
  UInt32 CentralDirectoryStartOffset;
  UInt16 CommentSize;
};

struct CEndOfCentralDirectoryRecordFull
{
  UInt32 Signature;
  CEndOfCentralDirectoryRecord Header;
};
*/

namespace NFileHeader
{
  /*
  struct CVersion
  {
    Byte Version;
    Byte HostOS;
  };
  */
  
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
      
      kBZip2 = 12,
      kWzPPMd = 0x62,
      kWzAES = 0x63
    };
    const int kNumCompressionMethods = 11;
    const Byte kMadeByProgramVersion = 20;
    
    const Byte kDeflateExtractVersion = 20;
    const Byte kStoreExtractVersion = 10;
    
    const Byte kSupportedVersion   = 20;
  }

  namespace NExtraID
  {
    enum 
    { 
      kZip64 = 0x01,
      kStrongEncrypt = 0x17,
      kWzAES = 0x9901
    };
  }

  const UInt32 kLocalBlockSize = 26;
  /*
  struct CLocalBlock
  {
    CVersion ExtractVersion;
    
    UInt16 Flags;
    UInt16 CompressionMethod;
    UInt32 Time;
    UInt32 FileCRC;
    UInt32 PackSize;
    UInt32 UnPackSize;
    UInt16 NameSize;
    UInt16 ExtraSize;
  };
  */

  const UInt32 kDataDescriptorSize = 16;
  // const UInt32 kDataDescriptor64Size = 16 + 8;
  /*
  struct CDataDescriptor
  {
    UInt32 Signature;
    UInt32 FileCRC;
    UInt32 PackSize;
    UInt32 UnPackSize;
  };

  struct CLocalBlockFull
  {
    UInt32 Signature;
    CLocalBlock Header;
  };
  */
  
  const UInt32 kCentralBlockSize = 42;
  /*
  struct CBlock
  {
    CVersion MadeByVersion;
    CVersion ExtractVersion;
    UInt16 Flags;
    UInt16 CompressionMethod;
    UInt32 Time;
    UInt32 FileCRC;
    UInt32 PackSize;
    UInt32 UnPackSize;
    UInt16 NameSize;
    UInt16 ExtraSize;
    UInt16 CommentSize;
    UInt16 DiskNumberStart;
    UInt16 InternalAttributes;
    UInt32 ExternalAttributes;
    UInt32 LocalHeaderOffset;
  };
  
  struct CBlockFull
  {
    UInt32 Signature;
    CBlock Header;
  };
  */

  namespace NFlags 
  {
    const int kNumUsedBits = 4;
    const int kUsedBitsMask = (1 << kNumUsedBits) - 1;
    
    const int kEncrypted = 1 << 0;
    const int kDescriptorUsedMask = 1 << 3;
    const int kStrongEncrypted = 1 << 6;
    
    const int kImplodeDictionarySizeMask = 1 << 1;
    const int kImplodeLiteralsOnMask     = 1 << 2;
    
    const int kDeflateTypeBitStart = 1;
    const int kNumDeflateTypeBits = 2;
    const int kNumDeflateTypes = (1 << kNumDeflateTypeBits);
    const int kDeflateTypeMask = (1 << kNumDeflateTypeBits) - 1;
  }
  
  namespace NHostOS
  {
    enum EEnum
    {
        kFAT      = 0,
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
        kTandem   = 17,
        kOS400    = 18,
        kOSX      = 19
    };
  }
  namespace NUnixAttribute
  {
    const UInt32 kIFMT   =   0170000;     /* Unix file type mask */
    
    const UInt32 kIFDIR  =   0040000;     /* Unix directory */
    const UInt32 kIFREG  =   0100000;     /* Unix regular file */
    const UInt32 kIFSOCK =   0140000;     /* Unix socket (BSD, not SysV or Amiga) */
    const UInt32 kIFLNK  =   0120000;     /* Unix symbolic link (not SysV, Amiga) */
    const UInt32 kIFBLK  =   0060000;     /* Unix block special       (not Amiga) */
    const UInt32 kIFCHR  =   0020000;     /* Unix character special   (not Amiga) */
    const UInt32 kIFIFO  =   0010000;     /* Unix fifo    (BCC, not MSC or Amiga) */
    
    const UInt32 kISUID  =   04000;       /* Unix set user id on execution */
    const UInt32 kISGID  =   02000;       /* Unix set group id on execution */
    const UInt32 kISVTX  =   01000;       /* Unix directory permissions control */
    const UInt32 kENFMT  =   kISGID;   /* Unix record locking enforcement flag */
    const UInt32 kIRWXU  =   00700;       /* Unix read, write, execute: owner */
    const UInt32 kIRUSR  =   00400;       /* Unix read permission: owner */
    const UInt32 kIWUSR  =   00200;       /* Unix write permission: owner */
    const UInt32 kIXUSR  =   00100;       /* Unix execute permission: owner */
    const UInt32 kIRWXG  =   00070;       /* Unix read, write, execute: group */
    const UInt32 kIRGRP  =   00040;       /* Unix read permission: group */
    const UInt32 kIWGRP  =   00020;       /* Unix write permission: group */
    const UInt32 kIXGRP  =   00010;       /* Unix execute permission: group */
    const UInt32 kIRWXO  =   00007;       /* Unix read, write, execute: other */
    const UInt32 kIROTH  =   00004;       /* Unix read permission: other */
    const UInt32 kIWOTH  =   00002;       /* Unix write permission: other */
    const UInt32 kIXOTH  =   00001;       /* Unix execute permission: other */
  }
  
  namespace NAmigaAttribute
  {
    const UInt32 kIFMT     = 06000;       /* Amiga file type mask */
    const UInt32 kIFDIR    = 04000;       /* Amiga directory */
    const UInt32 kIFREG    = 02000;       /* Amiga regular file */
    const UInt32 kIHIDDEN  = 00200;       /* to be supported in AmigaDOS 3.x */
    const UInt32 kISCRIPT  = 00100;       /* executable script (text command file) */
    const UInt32 kIPURE    = 00040;       /* allow loading into resident memory */
    const UInt32 kIARCHIVE = 00020;       /* not modified since bit was last set */
    const UInt32 kIREAD    = 00010;       /* can be opened for reading */
    const UInt32 kIWRITE   = 00004;       /* can be opened for writing */
    const UInt32 kIEXECUTE = 00002;       /* executable image, a loadable runfile */
    const UInt32 kIDELETE  = 00001;      /* can be deleted */
  }
}

}}

#endif
