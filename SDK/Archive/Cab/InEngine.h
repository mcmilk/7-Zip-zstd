// Archive/Cab/InEngine.h

#pragma once

#ifndef __ARCHIVE_CAB_INENGINE_H
#define __ARCHIVE_CAB_INENGINE_H

#include "Interface/IInOutStreams.h"
#include "Archive/Cab/Header.h"
#include "Archive/Cab/ItemInfo.h"

namespace NArchive{
namespace NCab{

class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnexpectedEndOfArchive = 0,
    kIncorrectArchive,
    kUnsupported,
  } Cause;
  CInArchiveException(CCauseType aCause) : Cause(aCause) {}
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
  STDMETHOD(SetTotal)(const UINT64 *aNumFiles) PURE;
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles) PURE;
};

class CInArchive
{
public:
  HRESULT Open(IInStream *aStream, 
      const UINT64 *aSearchHeaderSizeLimit,
      CInArchiveInfo &anInArchiveInfo,
      CObjectVector<NHeader::CFolder> &aFolders,
      CObjectVector<CFileInfo> &aFiles,
      CProgressVirt *aProgressVirt);
};
  
}}
  
#endif
