// Archive/cpio/Header.h

#pragma once

#ifndef __ARCHIVE_TAR_HEADER_H
#define __ARCHIVE_TAR_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace Ncpio {

#pragma pack( push, PragmacpioHeaders)
#pragma pack( push, 1)

namespace NFileHeader
{
  namespace NMagic 
  {
    extern const char *kMagic1;
    extern const char *kMagic2;
    extern const char *kEndName;
  }

  struct CRecord
  {
    char Magic[6];  /* "070701" for "new" portable format, "070702" for CRC format */
    char inode[8];
    char Mode[8];
    char UID[8];
    char GID[8];
    char nlink[8];
    char mtime[8];
    char Size[8]; /* must be 0 for FIFOs and directories */
    char DevMajor[8];
    char DevMinor[8];
    char RDevMajor[8];  /*only valid for chr and blk special files*/
    char RDevMinor[8];  /*only valid for chr and blk special files*/
    char NameSize[8]; /*count includes terminating NUL in pathname*/
    char ChkSum[8];  /* 0 for "new" portable format; for CRC format the sum of all the bytes in the file  */
    int CheckMagic()
    { return memcmp(Magic, NMagic::kMagic1, 6) == 0 || 
             memcmp(Magic, NMagic::kMagic2, 6) == 0;  };
  };
}

#pragma pack(pop)
#pragma pack(pop, PragmacpioHeaders)

}}

#endif
