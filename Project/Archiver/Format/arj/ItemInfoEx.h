// Archive/Arj/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_ARJ_ITEMINFOEX_H
#define __ARCHIVE_ARJ_ITEMINFOEX_H

#include "ItemInfo.h"

namespace NArchive {
namespace NArj {
  
class CItemInfoEx: public CItemInfo
{
public:
  UINT64 DataPosition;
};

}}

#endif



