// SortUtils.cpp

#include <StdAfx.h>

#include "SortUtils.h"

static int CompareStrings( const void *anElem1, const void *anElem2)
{
  const UString &aString1 = *(*(*((const UString ***)anElem1)));
  const UString &aString2 = *(*(*((const UString ***)anElem2)));
  return aString1.CompareNoCase(aString2);
}

void SortStringsToIndexes(UStringVector &aStrings, std::vector<int> &anIndexes)
{
  anIndexes.clear();
  if (aStrings.IsEmpty())
    return;
  int aNumItems = aStrings.Size();
  CPointerVector aPointers;
  aPointers.Reserve(aNumItems);
  int i;
  for(i = 0; i < aNumItems; i++)
    aPointers.Add(&aStrings.CPointerVector::operator[](i));
  void **aStringsBase  = (void **)aPointers[0];
  qsort(&aPointers[0], aNumItems, sizeof(void *), CompareStrings);
  for(i = 0; i < aNumItems; i++)
    anIndexes.push_back((void **)aPointers[i] - aStringsBase);
}
