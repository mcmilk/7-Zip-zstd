// Zip/UpdateItemInfo.h

#pragma once

#ifndef __ZIP_UPDATEITEMINFO_H
#define __ZIP_UPDATEITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Common/Vector.h"

namespace NArchive {
namespace NZip{

struct CUpdateRange
{
  UINT32 Position; 
  UINT32 Size;
  CUpdateRange() {};
  CUpdateRange(UINT32 aPosition, UINT32 aSize):
      Position(aPosition), Size(aSize) {};
};

struct CUpdateItemInfo
{
  bool NewData;
  bool NewProperties;
  bool IsDirectory;
  int IndexInArchive;
  int IndexInClient;
  UINT32 Attributes;
  UINT32 Time;
  UINT32 Size;
  AString Name;
  // bool ExistInArchive;
  bool Commented;
  CUpdateRange CommentRange;
  /*
  bool IsDirectory() const 
    { return ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0); };
  */
};

}}

#endif