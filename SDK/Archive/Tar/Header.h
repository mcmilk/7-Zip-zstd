// Archive/Tar/Header.h

#pragma once

#ifndef __ARCHIVE_TAR_HEADER_H
#define __ARCHIVE_TAR_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NTar {

#pragma pack( push, PragmaTarHeaders)
#pragma pack( push, 1)

namespace NFileHeader
{
  const kRecordSize = 512;
  const kNameSize = 100;
  const kUserNameSize = 32;
  const kGroupNameSize = 32;

  struct CHeader
  {
    char Name[kNameSize];
    char Mode[8];
    char UID[8];
    char GID[8];
    char Size[12];
    char ModificationTime[12];
    char CheckSum[8];
    char LinkFlag;
    char LinkName[kNameSize];
    char Magic[8];
    char UserName[kUserNameSize];
    char GroupName[kGroupNameSize];
    char DeviceMajor[8];
    char DeviceMinor[8];
    /*
    BYTE Padding[kRecordSize - (kNameSize + 8 + 8 + 12 + 12 + 8 + 1 + kNameSize + 8 +
      kUserNameSize + kGroupNameSize + 8 + 8)];
    */
  };
  union CRecord
  {
    CHeader Header;
    BYTE Padding[kRecordSize];
  };


  namespace NMode
  {
    const kSetUID   = 04000;  // Set UID on execution
    const kSetGID   = 02000;  // Set GID on execution 
    const kSaveText = 01000;  // Save text (sticky bit)
  }

  namespace NFilePermissions
  {
    const kUserRead     = 00400;  // read by owner
    const kUserWrite    = 00200;  // write by owner
    const kUserExecute  = 00100;  // execute/search by owner
    const kGroupRead    = 00040;  // read by group
    const kGroupWrite   = 00020;  // write by group
    const kGroupExecute = 00010;  // execute/search by group
    const kOtherRead    = 00004;  // read by other
    const kOtherWrite   = 00002;  // write by other
    const kOtherExecute = 00001;  // execute/search by other
  }


  // The linkflag defines the type of file
  namespace NLinkFlag
  {
    const char kOldNormal    = '\0'; // Normal disk file, Unix compatible
    const char kNormal       = '0'; // Normal disk file 
    const char kLink         = '1'; // Link to previously dumped file
    const char kSymbolicLink = '2'; // Symbolic link
    const char kCharacter    = '3'; // Character special file
    const char kBlock        = '4'; // Block special file
    const char kDirectory    = '5'; // Directory
    const char kFIFO         = '6'; // FIFO special file
    const char kContiguous   = '7'; // Contiguous file
  }
  // Further link types may be defined later.

  // The checksum field is filled with this while the checksum is computed.
  extern const char *kCheckSumBlanks;//   = "        ";   // 8 blanks, no null

  // The magic field is filled with this if uname and gname are valid.
  namespace NMagic 
  {
    extern const char *kUsTar; //   = "ustar  "; // 7 chars and a null
    extern const char *kGNUTar; //  = "GNUtar "; // 7 chars and a null
    extern const char *kEmpty; //  = "GNUtar "; // 7 chars and a null
  }

}

#pragma pack(pop)
#pragma pack(pop, PragmaTarHeaders)

}}

#endif
