// Archive/CabIn.h

#ifndef __ARCHIVE_CAB_IN_H
#define __ARCHIVE_CAB_IN_H

#include "../../IStream.h"
#include "CabHeader.h"
#include "CabItem.h"

namespace NArchive {
namespace NCab {

class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kIncorrectArchive,
    kUnsupported,
  } Cause;
  CInArchiveException(CCauseType cause) : Cause(cause) {}
};

class CInArchiveInfo
{
public:
  UInt32  Size;	/* size of this cabinet file in bytes */
  Byte  VersionMinor;	/* cabinet file format version, minor */
  Byte  VersionMajor;	/* cabinet file format version, major */
  UInt16  NumFolders;	/* number of CFFOLDER entries in this cabinet */
  UInt16  NumFiles;	/* number of CFFILE entries in this cabinet */
  UInt16  Flags;	/* cabinet file option indicators */
  UInt16  SetID;	/* must be the same for all cabinets in a set */
  UInt16  CabinetNumber;	/* number of this cabinet file in a set */

  bool ReserveBlockPresent() const { return (Flags & NHeader::NArchive::NFlags::kReservePresent) != 0; }
  NHeader::NArchive::CPerDataSizes PerDataSizes;

  AString PreviousCabinetName;
  AString PreviousDiskName;
  AString NextCabinetName;
  AString NextDiskName;
};

class CProgressVirt
{
public:
  STDMETHOD(SetTotal)(const UInt64 *numFiles) PURE;
  STDMETHOD(SetCompleted)(const UInt64 *numFiles) PURE;
};

const UInt32 kMaxBlockSize = NHeader::NArchive::kArchiveHeaderSize;

class CInArchive
{
  UInt16 _blockSize;
  Byte _block[kMaxBlockSize];
  UInt32 _blockPos;

  Byte ReadByte();
  UInt16 ReadUInt16();
  UInt32 ReadUInt32();
public:
  HRESULT Open(IInStream *inStream, 
      const UInt64 *searchHeaderSizeLimit,
      CInArchiveInfo &inArchiveInfo,
      CObjectVector<NHeader::CFolder> &folders,
      CObjectVector<CItem> &files,
      CProgressVirt *progressVirt);
};
  
}}
  
#endif
