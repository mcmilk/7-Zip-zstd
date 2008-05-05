// Archive/LzmaItem.h

#ifndef __ARCHIVE_LZMA_ITEM_H
#define __ARCHIVE_LZMA_ITEM_H

#include "Common/Types.h"

#include "../../../../C/CpuArch.h"

namespace NArchive {
namespace NLzma {

struct CHeader
{
  UInt64 UnpackSize;
  bool IsThereFilter;
  Byte FilterMethod;
  Byte LzmaProps[5];

  UInt32 GetDicSize() const { return GetUi32(LzmaProps + 1); }
  bool HasUnpackSize() const { return (UnpackSize != (UInt64)(Int64)-1);  }
  unsigned GetHeaderSize() const { return 5 + 8 + (IsThereFilter ? 1 : 0); }
};

}}

#endif
