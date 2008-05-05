// DirItem.h

#ifndef __DIR_ITEM_H
#define __DIR_ITEM_H

#include "Common/MyString.h"
#include "Common/Types.h"
#include "../../Archive/IArchive.h"

struct CDirItem
{ 
  FILETIME CreationTime;
  FILETIME LastAccessTime;
  FILETIME LastWriteTime;
  UInt64 Size;
  UString Name;
  UString FullPath;
  UInt32 Attributes;
  bool IsDirectory() const { return (Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ; }
};

struct CArchiveItem
{ 
  FILETIME LastWriteTime;
  UInt64 Size;
  UString Name;
  bool IsDirectory;
  bool SizeIsDefined;
  bool Censored;
  UInt32 IndexInServer;
  int FileTimeType;
  CArchiveItem(): IsDirectory(false), SizeIsDefined(false), Censored(false), FileTimeType(-1) {}
};

#endif
