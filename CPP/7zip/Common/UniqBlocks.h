// UniqBlocks.h

#ifndef __UNIQ_BLOCKS_H
#define __UNIQ_BLOCKS_H

#include "../../Common/MyTypes.h"
#include "../../Common/MyBuffer.h"
#include "../../Common/MyVector.h"

struct CUniqBlocks
{
  CObjectVector<CByteBuffer> Bufs;
  CIntVector Sorted;
  CIntVector BufIndexToSortedIndex;

  int AddUniq(const Byte *data, size_t size);
  UInt64 GetTotalSizeInBytes() const;
  void GetReverseMap();

  bool IsOnlyEmpty() const
  {
    if (Bufs.Size() == 0)
      return true;
    if (Bufs.Size() > 1)
      return false;
    return Bufs[0].Size() == 0;
  }
};

#endif
