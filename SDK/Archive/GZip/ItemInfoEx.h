// Archive/GZip/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_GZIP_ITEMINFOEX_H
#define __ARCHIVE_GZIP_ITEMINFOEX_H

#include "Common/Vector.h"

#include "Archive/GZip/ItemInfo.h"

namespace NArchive {
namespace NGZip {
  
class CItemInfoEx: public CItemInfo
{
public:
  UINT64 DataPosition;
  UINT64 CommentPosition;
  UINT64 ExtraPosition;
};

}}

#endif



