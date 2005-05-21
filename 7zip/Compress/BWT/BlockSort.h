// BlockSort.h

#ifndef __BLOCKSORT_H
#define __BLOCKSORT_H

#include "Common/Types.h"

namespace NCompress {

class CBlockSorter
{
  UInt32 *Groups;
  UInt32 *Flags;
  UInt32 BlockSize;
  UInt32 NumSortedBytes;
  UInt32 BlockSizeMax;
  UInt32 SortGroup(UInt32 groupOffset, UInt32 groupSize, UInt32 mask, UInt32 maskSize);
public:
  UInt32 *Indices;
  CBlockSorter(): Indices(0) {} 
  ~CBlockSorter() { Free(); }
  bool Create(UInt32 blockSizeMax);
  void Free();
  UInt32 Sort(const Byte *data, UInt32 blockSize);
};

}

#endif
