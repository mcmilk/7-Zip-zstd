// DirItem.h

#pragma once

#ifndef __DIR_ITEM_H
#define __DIR_ITEM_H

// #include "Common/Types.h"
#include "Common/String.h"
// #include "Windows/PropVariant.h"

struct CDirItem
{ 
  UINT32 Attributes;
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  UINT64 Size;
  UString Name;
  UString FullPath;
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

struct CArchiveItem
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

#endif
