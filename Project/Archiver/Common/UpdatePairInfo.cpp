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

static int MyCompareTime(NFileTimeType::EEnum aFileTimeType, 
    const FILETIME &aTime1, const FILETIME &aTime2)
{
  switch(aFileTimeType)
  {
    case NFileTimeType::kWindows:
      return ::CompareFileTime(&aTime1, &aTime2);
    case NFileTimeType::kUnix:
      {
        time_t anUnixTime1, anUnixTime2;
        if (!FileTimeToUnixTime(aTime1, anUnixTime1))
          throw 4191614;
        if (!FileTimeToUnixTime(aTime2, anUnixTime2))
          throw 4191615;
        return MyCompare(anUnixTime1, anUnixTime2);
      }
    case NFileTimeType::kDOS:
      {
        UINT32 aDOSTime1, aDOSTime2;
        if (!FileTimeToDosTime(aTime1, aDOSTime1))
          throw 4191616;
        if (!FileTimeToDosTime(aTime2, aDOSTime2))
          throw 4191617;
        return MyCompare(aDOSTime1, aDOSTime2);
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

static void TestDuplicateString(const UStringVector &aStrings, 
    const vector<int> &aIndexes)
{
  for(vector<int>::size_type i = 0; i + 1 < aIndexes.size(); i++)
    if (aStrings[aIndexes[i]].CollateNoCase(aStrings[aIndexes[i + 1]]) == 0)
    {
      UString aString = kDuplicateFileNameMessage;
      aString += L"\n";
      aString += aStrings[aIndexes[i]];
      aString += L"\n";
      aString += aStrings[aIndexes[i+1]];
      throw aString;
    }
}

void GetUpdatePairInfoList(const CArchiveStyleDirItemInfoVector &aDirItems, 
    const CArchiveItemInfoVector &anArchiveItems,
    NFileTimeType::EEnum aFileTimeType,
    CUpdatePairInfoVector &anUpdatePairs)
{
  vector<int> aDirIndexes, anArchiveIndexes;
  UStringVector aDirNames, anArchiveNames;
  
  int aNumDirItems = aDirItems.Size(); 
  int i;
  for(i = 0; i < aNumDirItems; i++)
    aDirNames.Add(aDirItems[i].Name);
  SortStringsToIndexes(aDirNames, aDirIndexes);
  TestDuplicateString(aDirNames, aDirIndexes);

  int aNumArchiveItems = anArchiveItems.Size(); 
  for(i = 0; i < aNumArchiveItems; i++)
    anArchiveNames.Add(anArchiveItems[i].Name);
  SortStringsToIndexes(anArchiveNames, anArchiveIndexes);
  TestDuplicateString(anArchiveNames, anArchiveIndexes);
  
  int aDirItemIndex = 0, anArchiveItemIndex = 0; 
  CUpdatePairInfo aPairInfo;
  while(aDirItemIndex < aNumDirItems && anArchiveItemIndex < aNumArchiveItems)
  {
    int aDirItemIndex2 = aDirIndexes[aDirItemIndex],
        anArchiveItemIndex2 = anArchiveIndexes[anArchiveItemIndex]; 
    const CArchiveStyleDirItemInfo &aDirItem = aDirItems[aDirItemIndex2];
    const CArchiveItemInfo &anArchiveItem = anArchiveItems[anArchiveItemIndex2];
    int aCompareResult = aDirItem.Name.CollateNoCase(anArchiveItem.Name);
    if (aCompareResult < 0)
    {
        aPairInfo.State = NUpdateArchive::NPairState::kOnlyOnDisk;
        aPairInfo.DirItemIndex = aDirItemIndex2;
        aDirItemIndex++;
    }
    else if (aCompareResult > 0)
    {
      aPairInfo.State = anArchiveItem.Censored ? 
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
      aPairInfo.ArchiveItemIndex = anArchiveItemIndex2;
      anArchiveItemIndex++;
    }
    else
    {
      if (!anArchiveItem.Censored)
        throw 1082022;; // TTString(kNotCensoredCollisionMessaged + aDirItem.Name);
      aPairInfo.DirItemIndex = aDirItemIndex2;
      aPairInfo.ArchiveItemIndex = anArchiveItemIndex2;
      switch (MyCompareTime(aFileTimeType, aDirItem.LastWriteTime, anArchiveItem.LastWriteTime))
      {
        case -1:
          aPairInfo.State = NUpdateArchive::NPairState::kNewInArchive;
          break;
        case 1:
          aPairInfo.State = NUpdateArchive::NPairState::kOldInArchive;
          break;
        default:
          if (anArchiveItem.SizeIsDefined)
            if (aDirItem.Size != anArchiveItem.Size)
              // throw 1082034; // kSameTimeChangedSizeCollisionMessaged;
              aPairInfo.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
            else
              aPairInfo.State = NUpdateArchive::NPairState::kSameFiles;
          else
              aPairInfo.State = NUpdateArchive::NPairState::kUnknowNewerFiles;
      }
      aDirItemIndex++;
      anArchiveItemIndex++;
    }
    anUpdatePairs.Add(aPairInfo);
  }
  for(;aDirItemIndex < aNumDirItems; aDirItemIndex++)
  {
    aPairInfo.State = NUpdateArchive::NPairState::kOnlyOnDisk;
    aPairInfo.DirItemIndex = aDirIndexes[aDirItemIndex];
    anUpdatePairs.Add(aPairInfo);
  }
  for(;anArchiveItemIndex < aNumArchiveItems; anArchiveItemIndex++)
  {
    int anArchiveItemIndex2 = anArchiveIndexes[anArchiveItemIndex]; 
    const CArchiveItemInfo &anArchiveItem = anArchiveItems[anArchiveItemIndex2];
    aPairInfo.State = anArchiveItem.Censored ?  
        NUpdateArchive::NPairState::kOnlyInArchive: NUpdateArchive::NPairState::kNotMasked;
    aPairInfo.ArchiveItemIndex = anArchiveItemIndex2;
    anUpdatePairs.Add(aPairInfo);
  }
}
