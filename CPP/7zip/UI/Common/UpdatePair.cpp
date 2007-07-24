// UpdatePair.cpp

#include "StdAfx.h"

#include <time.h>

#include "Common/Defs.h"
#include "Common/Wildcard.h"
#include "Windows/Time.h"

#include "UpdatePair.h"
#include "SortUtils.h"

using namespace NWindows;
using namespace NTime;

static int MyCompareTime(NFileTimeType::EEnum fileTimeType, 
    const FILETIME &time1, const FILETIME &time2)
{
  switch(fileTimeType)
  {
    case NFileTimeType::kWindows:
      return ::CompareFileTime(&time1, &time2);
    case NFileTimeType::kUnix:
      {
        UInt32 unixTime1, unixTime2;
        if (!FileTimeToUnixTime(time1, unixTime1))
        {
          unixTime1 = 0;
          // throw 4191614;
        }
        if (!FileTimeToUnixTime(time2, unixTime2))
        {
          unixTime2 = 0;
          // throw 4191615;
        }
        return MyCompare(unixTime1, unixTime2);
      }
    case NFileTimeType::kDOS:
      {
        UInt32 dosTime1, dosTime2;
        FileTimeToDosTime(time1, dosTime1);
        FileTimeToDosTime(time2, dosTime2);
        /*
        if (!FileTimeToDosTime(time1, dosTime1))
          throw 4191616;
        if (!FileTimeToDosTime(time2, dosTime2))
          throw 4191617;
        */
        return MyCompare(dosTime1, dosTime2);
      }
  }
  throw 4191618;
}

static const wchar_t *kDuplicateFileNameMessage = L"Duplicate filename:";

/*
static const char *kNotCensoredCollisionMessaged = "Internal file name collision:\n";
static const char *kSameTimeChangedSizeCollisionMessaged = 
    "Collision between files with same date/time and different sizes:\n";
*/

static void TestDuplicateString(const UStringVector &strings, const CIntVector &indices)
{
  for(int i = 0; i + 1 < indices.Size(); i++)
    if (CompareFileNames(strings[indices[i]], strings[indices[i + 1]]) == 0)
    {
      UString message = kDuplicateFileNameMessage;
      message += L"\n";
      message += strings[indices[i]];
      message += L"\n";
      message += strings[indices[i + 1]];
      throw message;
    }
}

void GetUpdatePairInfoList(
    const CObjectVector<CDirItem> &dirItems, 
    const CObjectVector<CArchiveItem> &archiveItems,
    NFileTimeType::EEnum fileTimeType,
    CObjectVector<CUpdatePair> &updatePairs)
{
  CIntVector dirIndices, archiveIndices;
  UStringVector dirNames, archiveNames;
  
  int numDirItems = dirItems.Size(); 
  int i;
  for(i = 0; i < numDirItems; i++)
    dirNames.Add(dirItems[i].Name);
  SortFileNames(dirNames, dirIndices);
  TestDuplicateString(dirNames, dirIndices);

  int numArchiveItems = archiveItems.Size(); 
  for(i = 0; i < numArchiveItems; i++)
    archiveNames.Add(archiveItems[i].Name);
  SortFileNames(archiveNames, archiveIndices);
  TestDuplicateString(archiveNames, archiveIndices);
  
  int dirItemIndex = 0, archiveItemIndex = 0; 
  CUpdatePair pair;
  while(dirItemIndex < numDirItems && archiveItemIndex < numArchiveItems)
  {
    int dirItemIndex2 = dirIndices[dirItemIndex],
        archiveItemIndex2 = archiveIndices[archiveItemIndex]; 
    const CDirItem &dirItem = dirItems[dirItemIndex2];
    const CArchiveItem &archiveItem = archiveItems[archiveItemIndex2];
    int compareResult = CompareFileNames(dirItem.Name, archiveItem.Name);
    if (compareResult < 0)
    {
        pair.State = NUpdateArchive::NPairState::kOnlyOnDisk;
        pair.DirItemIndex = dirItemIndex2;
        dirItemIndex++;
    }
    else if (compareResult > 0)
    {
      pair.State = archiveItem.Censored ? 
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
      pair.ArchiveItemIndex = archiveItemIndex2;
      archiveItemIndex++;
    }
    else
    {
      if (!archiveItem.Censored)
        throw 1082022;; // TTString(kNotCensoredCollisionMessaged + dirItem.Name);
      pair.DirItemIndex = dirItemIndex2;
      pair.ArchiveItemIndex = archiveItemIndex2;
      switch (MyCompareTime(fileTimeType, dirItem.LastWriteTime, archiveItem.LastWriteTime))
      {
        case -1:
          pair.State = NUpdateArchive::NPairState::kNewInArchive;
          break;
        case 1:
          pair.State = NUpdateArchive::NPairState::kOldInArchive;
          break;
        default:
          if (archiveItem.SizeIsDefined)
            if (dirItem.Size != archiveItem.Size)
              // throw 1082034; // kSameTimeChangedSizeCollisionMessaged;
              pair.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
            else
              pair.State = NUpdateArchive::NPairState::kSameFiles;
          else
              pair.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
      }
      dirItemIndex++;
      archiveItemIndex++;
    }
    updatePairs.Add(pair);
  }
  for(;dirItemIndex < numDirItems; dirItemIndex++)
  {
    pair.State = NUpdateArchive::NPairState::kOnlyOnDisk;
    pair.DirItemIndex = dirIndices[dirItemIndex];
    updatePairs.Add(pair);
  }
  for(;archiveItemIndex < numArchiveItems; archiveItemIndex++)
  {
    int archiveItemIndex2 = archiveIndices[archiveItemIndex]; 
    const CArchiveItem &archiveItem = archiveItems[archiveItemIndex2];
    pair.State = archiveItem.Censored ?  
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
    pair.ArchiveItemIndex = archiveItemIndex2;
    updatePairs.Add(pair);
  }
}
