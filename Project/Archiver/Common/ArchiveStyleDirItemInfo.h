// ArchiveStyleDirItemInfo.h

#pragma once

#ifndef __ARCHIVESTYLEDIRITEMINFO_H
#define __ARCHIVESTYLEDIRITEMINFO_H

#include "Common/Types.h"
#include "Common/Vector.h"
#include "Common/String.h"

#include "Windows/PropVariant.h"

struct CArchiveStyleDirItemInfo
{ 
  UINT32 Attributes;
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  UINT64 Size;
  UString Name;
  CSysString FullPathDiskName;
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

typedef CObjectVector<CArchiveStyleDirItemInfo> CArchiveStyleDirItemInfoVector;

struct CArchiveItemInfo
{ 
  bool IsDirectory;
  // DWORD Attributes;
  // NWindows::NCOM::CPropVariant LastWriteTime;
  FILETIME LastWriteTime;
  bool SizeIsDefined;
  UINT64 Size;
  UString Name;
  bool Censored;
  int IndexInServer;
};

typedef CObjectVector<CArchiveItemInfo> CArchiveItemInfoVector;


#endif
