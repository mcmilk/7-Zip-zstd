// Archive/cpio/ItemInfoEx.h

#pragma once

#ifndef __ARCHIVE_cpio_ITEMINFOEX_H
#define __ARCHIVE_cpio_ITEMINFOEX_H

#include "Common/Vector.h"

#include "Archive/cpio/ItemInfo.h"
#include "Archive/cpio/Header.h"

namespace NArchive {
namespace Ncpio {
  
class CItemInfoEx: public CItemInfo
{
public:
  UINT64 HeaderPosition;
  UINT64 HeaderSize;
  UINT64 GetDataPosition() const { return HeaderPosition + HeaderSize; };
};

}}

#endif



