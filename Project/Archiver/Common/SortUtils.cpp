// SortUtils.cpp

#include <StdAfx.h>

#include "SortUtils.h"

static int CompareStrings(const void *a1, const void *a2)
{
  const UString &s1 = *(*(*((const UString ***)a1)));
  const UString &s2 = *(*(*((const UString ***)a2)));
  return s1.CompareNoCase(s2);
}

void SortStringsToIndexes(UStringVector &strings, CIntVector &indices)
{
  indices.Clear();
  if (strings.IsEmpty())
    return;
  int numItems = strings.Size();
  CPointerVector pointers;
  pointers.Reserve(numItems);
  indices.Reserve(numItems);
  int i;
  for(i = 0; i < numItems; i++)
    pointers.Add(&strings.CPointerVector::operator[](i));
  void **stringsBase  = (void **)pointers[0];
  qsort(&pointers[0], numItems, sizeof(void *), CompareStrings);
  for(i = 0; i < numItems; i++)
    indices.Add((void **)pointers[i] - stringsBase);
}
