// 7z/UpdateItemInfo.h

#pragma once

#ifndef __7Z_UPDATEITEMINFO_H
#define __7Z_UPDATEITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Common/Vector.h"

namespace NArchive {
namespace N7z {

struct CUpdateRange
{
  UINT64 Position; 
  UINT64 Size;
  CUpdateRange() {};
  CUpdateRange(UINT32 aPosition, UINT32 aSize):
      Position(aPosition), Size(aSize) {};
};

struct CUpdateItemInfo
{
  int IndexInClient;
  UINT32 Attributes;
  FILETIME CreationTime;
  FILETIME LastWriteTime;

  UINT64 Size;
  UString Name;
  bool ExistInArchive;
  bool Commented;
  CUpdateRange CommentRange;
  bool IsDirectory() const 
    { return ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0); };
};

// typedef CObjectVector<CUpdateItemInfo> CUpdateRangeVector;
typedef CObjectVector<CUpdateItemInfo> CUpdateItemInfoVector;

}}


#endif