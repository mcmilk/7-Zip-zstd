// Archive/Cab/Header.h

#ifndef __ARCHIVE_CAB_HEADER_H
#define __ARCHIVE_CAB_HEADER_H

#include "Common/Types.h"

namespace NArchive {
namespace NCab {
namespace NHeader{

namespace NArchive {
  
  extern UInt32 kSignature;
  
  namespace NFlags
  {
    const int kPrevCabinet = 0x0001;
    const int kNextCabinet = 0x0002;
    const int kReservePresent = 0x0004;
  }

  const UInt32 kArchiveHeaderSize = 36;
  /*
  struct CBlock
  {
    UInt32  Signature;	// cabinet file signature
    UInt32  Reserved1;	// reserved
    UInt32  Size;	// size of this cabinet file in bytes
    UInt32  Reserved2;	// reserved
    UInt32  FileOffset;	// offset of the first CFFILE entry
    UInt32  Reserved3;	// reserved
    Byte  VersionMinor;	// cabinet file format version, minor
    Byte  VersionMajor;	// cabinet file format version, major
    UInt16  NumFolders;	// number of CFFOLDER entries in this cabinet
    UInt16  NumFiles;	// number of CFFILE entries in this cabinet
    UInt16  Flags;	// cabinet file option indicators
    UInt16  SetID;	// must be the same for all cabinets in a set
    UInt16  CabinetNumber;	// number of this cabinet file in a set
  };
  */

  const UInt32 kPerDataSizesHeaderSize = 4;

  struct CPerDataSizes
  {
    UInt16 PerCabinetAreaSize; 	// (optional) size of per-cabinet reserved area
    Byte PerFolderAreaSize; 	// (optional) size of per-folder reserved area
    Byte PerDatablockAreaSize; 	// (optional) size of per-datablock reserved area
  };

    /*
    Byte  abReserve[];	// (optional) per-cabinet reserved area
    Byte  szCabinetPrev[];	// (optional) name of previous cabinet file
    Byte  szDiskPrev[];	// (optional) name of previous disk
    Byte  szCabinetNext[];	// (optional) name of next cabinet file
    Byte  szDiskNext[];	// (optional) name of next disk
    */
}

namespace NCompressionMethodMajor
{
  const Byte kNone = 0;
  const Byte kMSZip = 1;
  const Byte kQuantum = 2;
  const Byte kLZX = 3;
}

const UInt32 kFolderHeaderSize = 8;
struct CFolder
{
  UInt32 DataStart;	// offset of the first CFDATA block in this folder
  UInt16 NumDataBlocks;	// number of CFDATA blocks in this folder
  Byte CompressionTypeMajor;
  Byte CompressionTypeMinor;
  // Byte  abReserve[];	// (optional) per-folder reserved area
  Byte GetCompressionMethod() const { return CompressionTypeMajor & 0xF; }
};

const int kFileNameIsUTFAttributeMask = 0x80;

namespace NFolderIndex
{
  const int kContinuedFromPrev    = 0xFFFD;
  const int kContinuedToNext      = 0xFFFE;
  const int kContinuedPrevAndNext = 0xFFFF;
  inline UInt16 GetRealFolderIndex(UInt16 aNumFolders, UInt16 aFolderIndex)
  {
    switch(aFolderIndex)
    {
      case kContinuedFromPrev:
        return 0;
      case kContinuedToNext:
      case kContinuedPrevAndNext:
        return aNumFolders - 1;
      default:
        return aFolderIndex;
    }
  }
}

const UInt32 kFileHeaderSize = 16;
/*
struct CFile
{
  UInt32  UnPackSize;	// uncompressed size of this file in bytes
  UInt32  UnPackOffset;	// uncompressed offset of this file in the folder
  UInt16  FolderIndex;	// index into the CFFOLDER area
  UInt16  PureDate;
  UInt16  PureTime;	// Time
  UInt16  Attributes;	// attribute flags for this file
  Byte  szName[];	// name of this file
};
*/
}}}

#endif
