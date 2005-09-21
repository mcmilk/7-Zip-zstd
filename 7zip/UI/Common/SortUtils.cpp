// SortUtils.cpp

#include "StdAfx.h"

#include "SortUtils.h"

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
}

void SortStrings(const UStringVector &src, UStringVector &dest)
{
  CIntVector indices;
  SortStringsToIndices(src, indices);
  dest.Clear();
  dest.Reserve(indices.Size());
  for (int i = 0; i < indices.Size(); i++)
    dest.Add(src[indices[i]]);
}
