// Archive/GZip/Header.h

#ifndef __ARCHIVE_GZIP_HEADER_H
#define __ARCHIVE_GZIP_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NGZip {

extern UInt16 kSignature;
static const UInt32 kSignatureSize = 2;

namespace NFileHeader
{
  /*
  struct CBlock
  {
    UInt16 Id;
    Byte CompressionMethod;
    Byte Flags;
    UInt32 Time;
    Byte ExtraFlags;
    Byte HostOS;
  };
  */
  
  namespace NFlags 
  {
    const int kDataIsText = 1 << 0;
    const int kHeaderCRCIsPresent = 1 << 1;
    const int kExtraIsPresent = 1 << 2;
    const int kNameIsPresent = 1 << 3;
    const int kComentIsPresent = 1 << 4;
  }
  
  namespace NExtraFlags 
  {
    enum EEnum
    {
      kMaximum = 2,
      kFastest = 4
    };
  }
  
  namespace NCompressionMethod
  {
    const Byte kDeflate = 8;
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
        kTHEOS    = 18,

        kUnknown = 255
    };
    const int kNumHostSystems = 19;
  }
}

}}

#endif
