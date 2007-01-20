// Archive/Tar/Header.h

#ifndef __ARCHIVE_TAR_HEADER_H
#define __ARCHIVE_TAR_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NTar {

namespace NFileHeader
{
  const int kRecordSize = 512;
  const int kNameSize = 100;
  const int kUserNameSize = 32;
  const int kGroupNameSize = 32;
  const int kPrefixSize = 155;

  /*
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
    char Prefix[155];
  };
  union CRecord
  {
    CHeader Header;
    Byte Padding[kRecordSize];
  };
  */

  namespace NMode
  {
    const int kSetUID   = 04000;  // Set UID on execution
    const int kSetGID   = 02000;  // Set GID on execution 
    const int kSaveText = 01000;  // Save text (sticky bit)
  }

  namespace NFilePermissions
  {
    const int kUserRead     = 00400;  // read by owner
    const int kUserWrite    = 00200;  // write by owner
    const int kUserExecute  = 00100;  // execute/search by owner
    const int kGroupRead    = 00040;  // read by group
    const int kGroupWrite   = 00020;  // write by group
    const int kGroupExecute = 00010;  // execute/search by group
    const int kOtherRead    = 00004;  // read by other
    const int kOtherWrite   = 00002;  // write by other
    const int kOtherExecute = 00001;  // execute/search by other
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

  extern const char *kLongLink;  //   = "././@LongLink";
  extern const char *kLongLink2; //   = "@LongLink";

  // The magic field is filled with this if uname and gname are valid.
  namespace NMagic 
  {
    extern const char *kUsTar; //   = "ustar"; // 5 chars
    extern const char *kGNUTar; //  = "GNUtar "; // 7 chars and a null
    extern const char *kEmpty; //  = "GNUtar "; // 7 chars and a null
  }

}

}}

#endif
