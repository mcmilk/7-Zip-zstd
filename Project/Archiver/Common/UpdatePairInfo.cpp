// UpdatePairInfoUtils.cpp

#include "StdAfx.h"

#include "UpdatePairInfo.h"
#include "../Common/SortUtils.h"
#include "Common/Defs.h"
#include "Windows/Time.h"

using namespace std;
using namespace NWindows;
using namespace NCOM;
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
        time_t unixTime1, unixTime2;
        if (!FileTimeToUnixTime(time1, unixTime1))
          throw 4191614;
        if (!FileTimeToUnixTime(time2, unixTime2))
          throw 4191615;
        return MyCompare(unixTime1, unixTime2);
      }
    case NFileTimeType::kDOS:
      {
        UINT32 dosTime1, dosTime2;
        if (!FileTimeToDosTime(time1, dosTime1))
          throw 4191616;
        if (!FileTimeToDosTime(time2, dosTime2))
          throw 4191617;
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

static void TestDuplicateString(const UStringVector &strings, 
    const CIntVector &indices)
{
  for(int i = 0; i + 1 < indices.Size(); i++)
    if (strings[indices[i]].CollateNoCase(strings[indices[i + 1]]) == 0)
    {
      UString message = kDuplicateFileNameMessage;
      message += L"\n";
      message += strings[indices[i]];
      message += L"\n";
      message += strings[indices[i+1]];
      throw message;
    }
}

void GetUpdatePairInfoList(const CArchiveStyleDirItemInfoVector &dirItems, 
    const CArchiveItemInfoVector &archiveItems,
    NFileTimeType::EEnum fileTimeType,
    CUpdatePairInfoVector &updatePairs)
{
  CIntVector dirIndices, archiveIndices;
  UStringVector dirNames, archiveNames;
  
  int numDirItems = dirItems.Size(); 
  int i;
  for(i = 0; i < numDirItems; i++)
    dirNames.Add(dirItems[i].Name);
  SortStringsToIndexes(dirNames, dirIndices);
  TestDuplicateString(dirNames, dirIndices);

  int numArchiveItems = archiveItems.Size(); 
  for(i = 0; i < numArchiveItems; i++)
    archiveNames.Add(archiveItems[i].Name);
  SortStringsToIndexes(archiveNames, archiveIndices);
  TestDuplicateString(archiveNames, archiveIndices);
  
  int dirItemIndex = 0, archiveItemIndex = 0; 
  CUpdatePairInfo pairInfo;
  while(dirItemIndex < numDirItems && archiveItemIndex < numArchiveItems)
  {
    int dirItemIndex2 = dirIndices[dirItemIndex],
        archiveItemIndex2 = archiveIndices[archiveItemIndex]; 
    const CArchiveStyleDirItemInfo &dirItem = dirItems[dirItemIndex2];
    const CArchiveItemInfo &archiveItem = archiveItems[archiveItemIndex2];
    int compareResult = dirItem.Name.CollateNoCase(archiveItem.Name);
    if (compareResult < 0)
    {
        pairInfo.State = NUpdateArchive::NPairState::kOnlyOnDisk;
        pairInfo.DirItemIndex = dirItemIndex2;
        dirItemIndex++;
    }
    else if (compareResult > 0)
    {
      pairInfo.State = archiveItem.Censored ? 
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
      pairInfo.ArchiveItemIndex = archiveItemIndex2;
      archiveItemIndex++;
    }
    else
    {
      if (!archiveItem.Censored)
        throw 1082022;; // TTString(kNotCensoredCollisionMessaged + dirItem.Name);
      pairInfo.DirItemIndex = dirItemIndex2;
      pairInfo.ArchiveItemIndex = archiveItemIndex2;
      switch (MyCompareTime(fileTimeType, dirItem.LastWriteTime, archiveItem.LastWriteTime))
      {
        case -1:
          pairInfo.State = NUpdateArchive::NPairState::kNewInArchive;
          break;
        case 1:
          pairInfo.State = NUpdateArchive::NPairState::kOldInArchive;
          break;
        default:
          if (archiveItem.SizeIsDefined)
            if (dirItem.Size != archiveItem.Size)
              // throw 1082034; // kSameTimeChangedSizeCollisionMessaged;
              pairInfo.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
            else
              pairInfo.State = NUpdateArchive::NPairState::kSameFiles;
          else
              pairInfo.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
      }
      dirItemIndex++;
      archiveItemIndex++;
    }
    updatePairs.Add(pairInfo);
  }
  for(;dirItemIndex < numDirItems; dirItemIndex++)
  {
    pairInfo.State = NUpdateArchive::NPairState::kOnlyOnDisk;
    pairInfo.DirItemIndex = dirIndices[dirItemIndex];
    updatePairs.Add(pairInfo);
  }
  for(;archiveItemIndex < numArchiveItems; archiveItemIndex++)
  {
    int archiveItemIndex2 = archiveIndices[archiveItemIndex]; 
    const CArchiveItemInfo &archiveItem = archiveItems[archiveItemIndex2];
    pairInfo.State = archiveItem.Censored ?  
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
    pairInfo.ArchiveItemIndex = archiveItemIndex2;
    updatePairs.Add(pairInfo);
  }
}
