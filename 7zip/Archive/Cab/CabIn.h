// Archive/CabIn.h

#pragma once

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
  UINT32  Size;	/* size of this cabinet file in bytes */
  BYTE  VersionMinor;	/* cabinet file format version, minor */
  BYTE  VersionMajor;	/* cabinet file format version, major */
  UINT16  NumFolders;	/* number of CFFOLDER entries in this cabinet */
  UINT16  NumFiles;	/* number of CFFILE entries in this cabinet */
  UINT16  Flags;	/* cabinet file option indicators */
  UINT16  SetID;	/* must be the same for all cabinets in a set */
  UINT16  CabinetNumber;	/* number of this cabinet file in a set */

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
  STDMETHOD(SetTotal)(const UINT64 *numFiles) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *numFiles) PURE;
};

class CInArchive
{
public:
  HRESULT Open(IInStream *inStream, 
      const UINT64 *searchHeaderSizeLimit,
      CInArchiveInfo &inArchiveInfo,
      CObjectVector<NHeader::CFolder> &folders,
      CObjectVector<CItem> &aFiles,
      CProgressVirt *aProgressVirt);
};
  
}}
  
#endif
