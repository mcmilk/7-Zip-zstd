// Windows/Time.h

#ifndef __WINDOWS_TIME_H
#define __WINDOWS_TIME_H

#include "Common/Types.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NTime {

inline bool DosTimeToFileTime(UInt32 dosTime, FILETIME &fileTime)
{
  return BOOLToBool(::DosDateTimeToFileTime(UInt16(dosTime >> 16), 
      UInt16(dosTime & 0xFFFF), &fileTime));
}

inline bool FileTimeToDosTime(const FILETIME &fileTime, UInt32 &dosTime)
{
  WORD datePart, timePart;
  if (!::FileTimeToDosDateTime(&fileTime, &datePart, &timePart))
    return false;
  dosTime = (((UInt32)datePart) << 16) + timePart;
  return true;
}

const UInt32 kNumTimeQuantumsInSecond = 10000000;
const UInt64 kUnixTimeStartValue = ((UInt64)kNumTimeQuantumsInSecond) * 60 * 60 * 24 * 134774;

inline void UnixTimeToFileTime(UInt32 unixTime, FILETIME &fileTime)
{
  UInt64 v = kUnixTimeStartValue + ((UInt64)unixTime) * kNumTimeQuantumsInSecond;
  fileTime.dwLowDateTime = (DWORD)v;
  fileTime.dwHighDateTime = (DWORD)(v >> 32);
}

inline bool FileTimeToUnixTime(const FILETIME &fileTime, UInt32 &unixTime)
{
  UInt64 winTime = (((UInt64)fileTime.dwHighDateTime) << 32) + fileTime.dwLowDateTime;
  if (winTime < kUnixTimeStartValue)
    return false;
  winTime = (winTime - kUnixTimeStartValue) / kNumTimeQuantumsInSecond;
  if (winTime > 0xFFFFFFFF)
    return false;
  unixTime = (UInt32)winTime;
  return true;
}

}}

#endif
