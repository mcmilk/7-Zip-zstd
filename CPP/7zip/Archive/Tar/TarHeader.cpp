// Archive/Tar/Header.h

#include "StdAfx.h"

#include "TarHeader.h"

namespace NArchive {
namespace NTar {
namespace NFileHeader {

  // The checksum field is filled with this while the checksum is computed.
  const char *kCheckSumBlanks = "        ";   // 8 blanks, no null

  const char *kLongLink = "././@LongLink";
  const char *kLongLink2 = "@LongLink";

  // The magic field is filled with this if uname and gname are valid.
  namespace NMagic 
  {
    const char *kUsTar  = "ustar"; // 5 chars
    const char *kGNUTar = "GNUtar "; // 7 chars and a null
    const char *kEmpty = "\0\0\0\0\0\0\0\0"; // 7 chars and a null
  }

}}}
