// UniqBlocks.cpp

#include "StdAfx.h"

#include "UniqBlocks.h"

int CUniqBlocks::AddUniq(const Byte *data, size_t size)
{
  unsigned left = 0, right = Sorted.Size();
  while (left != right)
  {
    unsigned mid = (left + right) / 2;
    int index = Sorted[mid];
    const CByteBuffer &buf = Bufs[index];
    size_t sizeMid = buf.Size();
    if (size < sizeMid)
      right = mid;
    else if (size > sizeMid)
      left = mid + 1;
    else
    {
      int cmp = memcmp(data, buf, size);
      if (cmp == 0)
        return index;
      if (cmp < 0)
        right = mid;
      else
        left = mid + 1;
    }
  }
  int index = Bufs.Size();
  Sorted.Insert(left, index);
  CByteBuffer &buf = Bufs.AddNew();
  buf.CopyFrom(data, size);
  return index;
}

UInt64 CUniqBlocks::GetTotalSizeInBytes() const
{
  UInt64 size = 0;
  FOR_VECTOR (i, Bufs)
    size += Bufs[i].Size();
  return size;
}

void CUniqBlocks::GetReverseMap()
{
  unsigned num = Sorted.Size();
  BufIndexToSortedIndex.ClearAndSetSize(num);
  int *p = &BufIndexToSortedIndex[0];
  unsigned i;
  for (i = 0; i < num; i++)
    p[i] = 0;
  for (i = 0; i < num; i++)
    p[Sorted[i]] = i;
}
