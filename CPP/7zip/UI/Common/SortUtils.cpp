// SortUtils.cpp

#include "StdAfx.h"

#include "SortUtils.h"
#include "Common/Types.h"

/*
template <class T>
void TSortRefDown(T *p, UInt32 k, UInt32 size, int (*compare)(const T*, const T*, void *), void *param)
{ 
  T temp = p[k];
  for (;;)
  {
    UInt32 s = (k << 1);
    if (s > size)
      break;
    if (s < size && compare(p + s + 1, p + s, param) > 0)
      s++;
    if (compare(&temp, p + s, param) >= 0)
      break;
    p[k] = p[s]; 
    k = s;
  } 
  p[k] = temp; 
}

template <class T>
void TSort(T* p, UInt32 size, int (*compare)(const T*, const T*, void *), void *param)
{
  if (size <= 1)
    return;
  p--;
  {
    UInt32 i = size / 2;
    do
      TSortRefDown(p, i, size, compare, param);
    while(--i != 0);
  }
  do
  {
    T temp = p[size];
    p[size--] = p[1];
    p[1] = temp;
    TSortRefDown(p, 1, size, compare, param);
  }
  while (size > 1);
}
*/

static int CompareStrings(const int *p1, const int *p2, void *param)
{
  const UStringVector &strings = *(const UStringVector *)param;
  const UString &s1 = strings[*p1];
  const UString &s2 = strings[*p2];
  return s1.CompareNoCase(s2);
}

void SortStringsToIndices(const UStringVector &strings, CIntVector &indices)
{
  indices.Clear();
  int numItems = strings.Size();
  indices.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
    indices.Add(i);
  indices.Sort(CompareStrings, (void *)&strings);
  // TSort(&indices.Front(), indices.Size(), CompareStrings, (void *)&strings);
}

/*
void SortStrings(const UStringVector &src, UStringVector &dest)
{
  CIntVector indices;
  SortStringsToIndices(src, indices);
  dest.Clear();
  dest.Reserve(indices.Size());
  for (int i = 0; i < indices.Size(); i++)
    dest.Add(src[indices[i]]);
}
*/
