// Archive/Cab/Header.h

#pragma once

#ifndef __ARCHIVE_CAB_HEADER_H
#define __ARCHIVE_CAB_HEADER_H

#include "Common/Types.h"

#pragma pack(push, PragmaCabHeaders)
#pragma pack(push, 1)

namespace NArchive{
namespace NCab{
namespace NHeader{

namespace NArchive {
  
  extern UINT32 kSignature;
  
  namespace NFlags
  {
    const kPrevCabinet = 0x0001;
    const kNextCabinet = 0x0002;
    const kReservePresent = 0x0004;
  }

  struct CBlock
  {
    UINT32  Signature;	/* cabinet file signature */
    UINT32  Reserved1;	/* reserved */
    UINT32  Size;	/* size of this cabinet file in bytes */
    UINT32  Reserved2;	/* reserved */
    UINT32  FileOffset;	/* offset of the first CFFILE entry */
    UINT32  Reserved3;	/* reserved */
    BYTE  VersionMinor;	/* cabinet file format version, minor */
    BYTE  VersionMajor;	/* cabinet file format version, major */
    UINT16  NumFolders;	/* number of CFFOLDER entries in this cabinet */
    UINT16  NumFiles;	/* number of CFFILE entries in this cabinet */
    UINT16  Flags;	/* cabinet file option indicators */
    UINT16  SetID;	/* must be the same for all cabinets in a set */
    UINT16  CabinetNumber;	/* number of this cabinet file in a set */
  };

  struct CPerDataSizes
  {
    UINT16  PerCabinetAreaSize; 	/* (optional) size of per-cabinet reserved area */
    BYTE  PerFolderAreaSize; 	/* (optional) size of per-folder reserved area */
    BYTE  PerDatablockAreaSize; 	/* (optional) size of per-datablock reserved area */
  };

    /*
    BYTE  abReserve[];	// (optional) per-cabinet reserved area
    BYTE  szCabinetPrev[];	// (optional) name of previous cabinet file
    BYTE  szDiskPrev[];	// (optional) name of previous disk
    BYTE  szCabinetNext[];	// (optional) name of next cabinet file
    BYTE  szDiskNext[];	// (optional) name of next disk
    */
}

namespace NCompressionMethodMajor
{
  const BYTE kNone = 0;
  const BYTE kMSZip = 1;
  const BYTE kQuantum = 2;
  const BYTE kLZX = 3;
}

struct CFolder
{
  UINT32  DataStart;	// offset of the first CFDATA block in this folder
  UINT16  NumDataBlocks;	// number of CFDATA blocks in this folder
  BYTE CompressionTypeMajor;
  BYTE CompressionTypeMinor;
  // BYTE  abReserve[];	// (optional) per-folder reserved area
};

const kFileNameIsUTFAttributeMask = 0x80;

namespace NFolderIndex
{
  const kContinuedFromPrev    = 0xFFFD;
  const kContinuedToNext      = 0xFFFE;
  const kContinuedPrevAndNext = 0xFFFF;
  inline UINT16 GetRealFolderIndex(UINT16 aNumFolders, UINT16 aFolderIndex)
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

struct CFile
{
  UINT32  UnPackSize;	/* uncompressed size of this file in bytes */
  UINT32  UnPackOffset;	/* uncompressed offset of this file in the folder */
  UINT16  FolderIndex;	/* index into the CFFOLDER area */
  UINT16  PureDate;
  UINT16  PureTime;	/* Time */
  UINT16  Attributes;	/* attribute flags for this file */
  //BYTE  szName[];	/* name of this file */
};

}}}

#pragma pack(pop)
#pragma pack(pop, PragmaCabHeaders)

#endif
