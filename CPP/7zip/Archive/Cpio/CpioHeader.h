// Archive/cpio/Header.h

#ifndef __ARCHIVE_CPIO_HEADER_H
#define __ARCHIVE_CPIO_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NCpio {

namespace NFileHeader
{
  namespace NMagic 
  {
    extern const char *kMagic1;
    extern const char *kMagic2;
    extern const char *kMagic3;
    extern const char *kEndName;
    extern const Byte kMagicForRecord2[2];
  }

  const UInt32 kRecord2Size = 26;
  /*
  struct CRecord2
  {
    unsigned short c_magic;
    short c_dev;
    unsigned short c_ino;
    unsigned short c_mode;
    unsigned short c_uid;
    unsigned short c_gid;
    unsigned short c_nlink;
    short c_rdev;
    unsigned short c_mtimes[2];
    unsigned short c_namesize;
    unsigned short c_filesizes[2];
  };
  */
 
  const UInt32 kRecordSize = 110;
  /*
  struct CRecord
  {
    char Magic[6];  // "070701" for "new" portable format, "070702" for CRC format
    char inode[8];
    char Mode[8];
    char UID[8];
    char GID[8];
    char nlink[8];
    char mtime[8];
    char Size[8]; // must be 0 for FIFOs and directories
    char DevMajor[8];
    char DevMinor[8];
    char RDevMajor[8];  //only valid for chr and blk special files
    char RDevMinor[8];  //only valid for chr and blk special files
    char NameSize[8]; // count includes terminating NUL in pathname
    char ChkSum[8];  // 0 for "new" portable format; for CRC format the sum of all the bytes in the file
    bool CheckMagic() const
    { return memcmp(Magic, NMagic::kMagic1, 6) == 0 || 
             memcmp(Magic, NMagic::kMagic2, 6) == 0;  };
  };
  */

  const UInt32 kOctRecordSize = 76;
  
}

}}

#endif
