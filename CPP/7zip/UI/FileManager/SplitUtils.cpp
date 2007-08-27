// SplitUtils.cpp

#include "StdAfx.h"

#include "Common/StringToInt.h"

#include "SplitUtils.h"
#include "StringUtils.h"

bool ParseVolumeSizes(const UString &s, CRecordVector<UInt64> &values)
{
  values.Clear();
  UStringVector destStrings;
  SplitString(s, destStrings);
  bool prevIsNumber = false;
  for (int i = 0; i < destStrings.Size(); i++)
  {
    UString subString = destStrings[i];
    subString.MakeUpper();
    if (subString.IsEmpty())
      return false;
    if (subString == L"-")
      return true;
    if (prevIsNumber)
    {
      wchar_t c = subString[0];
      UInt64 &value = values.Back();
      prevIsNumber = false;
      switch(c)
      {
        case L'B':
          continue;
        case L'K':
          value <<= 10;
          continue;
        case L'M':
          value <<= 20;
          continue;
        case L'G':
          value <<= 30;
          continue;
      }
    }
    const wchar_t *start = subString;
    const wchar_t *end;
    UInt64 value = ConvertStringToUInt64(start, &end);
    if (start == end)
      return false;
    if (value == 0)
      return false;
    values.Add(value);
    prevIsNumber = true;
    UString rem = subString.Mid((int)(end - start));
    if (!rem.IsEmpty())
      destStrings.Insert(i + 1, rem);
  }
  return true;
}

void AddVolumeItems(NWindows::NControl::CComboBox &volumeCombo)
{
  volumeCombo.AddString(TEXT("1457664 - 3.5\" floppy"));
  volumeCombo.AddString(TEXT("650M - CD"));
  volumeCombo.AddString(TEXT("700M - CD"));
  volumeCombo.AddString(TEXT("4480M - DVD"));
}

UInt64 GetNumberOfVolumes(UInt64 size, CRecordVector<UInt64> &volSizes)
{
  if (size == 0 || volSizes.Size() == 0)
    return 1;
  UInt64 numVolumes = 0;
  for (int i = 0; i < volSizes.Size(); i++)
  {
    UInt64 volSize = volSizes[i];
    numVolumes++;
    if (volSize >= size)
      return numVolumes;
    size -= volSize;
  }
  UInt64 volSize = volSizes.Back();
  if (volSize == 0)
    return (UInt64)(Int64)-1;
  return numVolumes + (size - 1) / volSize + 1;
}
