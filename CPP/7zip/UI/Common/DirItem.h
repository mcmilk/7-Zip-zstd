// DirItem.h

#ifndef __DIR_ITEM_H
#define __DIR_ITEM_H

#include "Common/MyString.h"
#include "Common/Types.h"

struct CDirItem
{ 
  UInt32 Attributes;
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  UInt64 Size;
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
  UInt64 Size;
  UString Name;
  bool Censored;
  int IndexInServer;
};

#endif
