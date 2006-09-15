// BlockSort.cpp

#include "StdAfx.h"

#include "BlockSort.h"

#include "Common/Alloc.h"

namespace NCompress {

static const int kNumHashBytes = 2;
static const UInt32 kNumHashValues = 1 << (kNumHashBytes * 8);

static const int kNumFlagsBits = 5; // 32 Flags in UInt32 word 
static const UInt32 kNumFlagsInWord = (1 << kNumFlagsBits);
static const UInt32 kFlagsMask = kNumFlagsInWord - 1;
static const UInt32 kAllFlags = 0xFFFFFFFF;

bool CBlockSorter::Create(UInt32 blockSizeMax)
{
  if (Indices != 0 && blockSizeMax == BlockSizeMax)
    return true;
  Free();
  BlockSizeMax = blockSizeMax;
  Indices = (UInt32 *)::BigAlloc((blockSizeMax * 2 + 
      ((blockSizeMax + kFlagsMask) >> kNumFlagsBits) + kNumHashValues) * sizeof(UInt32));
  return (Indices != 0);
}

void CBlockSorter::Free()
{
  ::BigFree(Indices);
  Indices = 0;
}

// SortGroup - is recursive Radix-Range-Sort function with Bubble-Sort optimization
// It uses both mask & maskSize (Range-Sort), since it can change values (Groups) 
// during sorting
// returns: 0 - if there are groups, 1 - no more groups
UInt32 CBlockSorter::SortGroup(UInt32 groupOffset, UInt32 groupSize, UInt32 mask, UInt32 maskSize)
{
  if (groupSize <= 2)
  {
    if (groupSize <= 1)
      return 0; 
    UInt32 *ind2 = Indices + groupOffset;
    UInt32 stringPos = ind2[0] + NumSortedBytes;
    if (stringPos >= BlockSize)
      stringPos -= BlockSize;
    UInt32 group = Groups[stringPos];
    stringPos = ind2[1] + NumSortedBytes;
    if (stringPos >= BlockSize)
      stringPos -= BlockSize;
    if (group == Groups[stringPos])
      return 1;
    if (group > Groups[stringPos])
    {
      UInt32 temp = ind2[0];
      ind2[0] = ind2[1];
      ind2[1] = temp;
    }
    Flags[groupOffset >> kNumFlagsBits] &= ~(1 << (groupOffset & kFlagsMask));
    Groups[ind2[1]] = groupOffset + 1;
    return 0;
  }

  // Check that all strings are in one group (cannot sort)
  UInt32 *ind2 = Indices + groupOffset;
  {
    UInt32 stringPos = ind2[0] + NumSortedBytes;
    if (stringPos >= BlockSize)
      stringPos -= BlockSize;
    UInt32 group = Groups[stringPos];
    UInt32 j;
    for (j = 1; j < groupSize; j++)
    {
      stringPos = ind2[j] + NumSortedBytes;
      if (stringPos >= BlockSize)
        stringPos -= BlockSize;
      if (Groups[stringPos] != group)
        break;
    }
    if (j == groupSize)
      return 1;
  }

  if (groupSize <= 15)
  {
    // Bubble-Sort
    UInt32 lastChange = groupSize;
    do
    {
      UInt32 stringPos = ind2[0] + NumSortedBytes;
      if (stringPos >= BlockSize)
        stringPos -= BlockSize;
      UInt32 group = Groups[stringPos];

      UInt32 sortSize = lastChange;
      lastChange = 0;
      for (UInt32 j = 1; j < sortSize; j++)
      {
        stringPos = ind2[j] + NumSortedBytes;
        if (stringPos >= BlockSize)
          stringPos -= BlockSize;
        if (Groups[stringPos] < group)
        {
          UInt32 temp = ind2[j];
          ind2[j] = ind2[j - 1];
          ind2[j - 1] = temp;
          lastChange = j;
        }
        else
          group = Groups[stringPos];
      }
    }
    while(lastChange > 1);

    // Write Flags
    UInt32 stringPos = ind2[0] + NumSortedBytes;
    if (stringPos >= BlockSize)
      stringPos -= BlockSize;
    UInt32 group = Groups[stringPos];

    UInt32 j;
    for (j = 1; j < groupSize; j++)
    {
      stringPos = ind2[j] + NumSortedBytes;
      if (stringPos >= BlockSize)
        stringPos -= BlockSize;
      if (Groups[stringPos] != group)
      {
        group = Groups[stringPos];
        UInt32 t = groupOffset + j - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
      }
    }

    // Write new Groups values and Check that there are groups
    UInt32 thereAreGroups = 0; 
    for (j = 0; j < groupSize; j++)
    {
      UInt32 group = groupOffset + j;
      for (;;)
      {
        Groups[ind2[j]] = group;
        if ((Flags[(groupOffset + j) >> kNumFlagsBits] & (1 << ((groupOffset + j) & kFlagsMask))) == 0)
          break;
        j++;
        thereAreGroups = 1;
      }
    }
    return thereAreGroups;
  }

  // Radix-Range Sort
  UInt32 i;
  for (;;)
  {
    if (maskSize == 0)
      return 1;
    UInt32 j = groupSize;
    i = 0;
    do
    {
      UInt32 stringPos = ind2[i] + NumSortedBytes;
      if (stringPos >= BlockSize)
        stringPos -= BlockSize;
      if (Groups[stringPos] >= mask)
      {
        for (j--; j > i; j--)
        {
          stringPos = ind2[j] + NumSortedBytes;
          if (stringPos >= BlockSize)
            stringPos -= BlockSize;
          if (Groups[stringPos] < mask)
          {
            UInt32 temp = ind2[i];
            ind2[i] = ind2[j];
            ind2[j] = temp;
            break;
          }
        }
        if (i >= j)
          break;
      }
    }
    while(++i < j);
    maskSize >>= 1;
    if (i == 0)
      mask += maskSize;
    else if (i == groupSize)
      mask -= maskSize;
    else 
      break;
  }
  UInt32 t = (groupOffset + i - 1);
  Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));

  for (UInt32 j = i; j < groupSize; j++)
    Groups[ind2[j]] = groupOffset + i;

  UInt32 res = SortGroup(groupOffset, i, mask - maskSize, maskSize);
  return res | SortGroup(groupOffset + i, groupSize - i, mask + maskSize, maskSize);
}

UInt32 CBlockSorter::Sort(const Byte *data, UInt32 blockSize)
{
  BlockSize = blockSize;
  UInt32 *counters = Indices + blockSize;
  Groups = counters + kNumHashValues;
  Flags = Groups + blockSize;
  UInt32 i;

  // Radix-Sort for 2 bytes
  for (i = 0; i < kNumHashValues; i++)
    counters[i] = 0;
  for (i = 0; i < blockSize - 1; i++)
    counters[((UInt32)data[i] << 8) | data[i + 1]]++;
  counters[((UInt32)data[i] << 8) | data[0]]++;

  {
    {
      UInt32 numWords = (blockSize + kFlagsMask) >> kNumFlagsBits;
      for (i = 0; i < numWords; i++)
        Flags[i] = kAllFlags;
    }

    UInt32 sum = 0;
    for (i = 0; i < kNumHashValues; i++)
    {
      UInt32 groupSize = counters[i];
      if (groupSize > 0)
      {
        UInt32 t = sum + groupSize - 1;
        Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
        sum += groupSize;
      }
      counters[i] = sum - groupSize;
    }

    for (i = 0; i < blockSize - 1; i++)
      Groups[i] = counters[((UInt32)data[i] << 8) | data[i + 1]];
    Groups[i] = counters[((UInt32)data[i] << 8) | data[0]];

    for (i = 0; i < blockSize - 1; i++)
      Indices[counters[((UInt32)data[i] << 8) | data[i + 1]]++] = i;
    Indices[counters[((UInt32)data[i] << 8) | data[0]]++] = i;
  }

  UInt32 mask;
  for (mask = 2; mask < blockSize; mask <<= 1);
  mask >>= 1;
  for (NumSortedBytes = kNumHashBytes; ; NumSortedBytes <<= 1)
  {
    UInt32 newLimit = 0;
    for (i = 0; i < blockSize;)
    {
      if ((Flags[i >> kNumFlagsBits] & (1 << (i & kFlagsMask))) == 0)
      {
        i++;
        continue;
      }
      UInt32 groupSize;
      for(groupSize = 1; 
        (Flags[(i + groupSize) >> kNumFlagsBits] & (1 << ((i + groupSize) & kFlagsMask))) != 0; 
        groupSize++);
      
      groupSize++;
      
      if (NumSortedBytes >= blockSize)
        for (UInt32 j = 0; j < groupSize; j++)
        {
          UInt32 t = (i + j);
          Flags[t >> kNumFlagsBits] &= ~(1 << (t & kFlagsMask));
          Groups[Indices[t]] = t;
        }
      else
        if (SortGroup(i, groupSize, mask, mask) != 0)
          newLimit = i + groupSize;
      i += groupSize;
    }
    if (newLimit == 0)
      break;
  }
  return Groups[0];
}

}
