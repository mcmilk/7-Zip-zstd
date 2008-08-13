// Windows/Time.cpp

#include "StdAfx.h"

#include "Time.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NTime {

bool DosTimeToFileTime(UInt32 dosTime, FILETIME &fileTime)
{
  return BOOLToBool(::DosDateTimeToFileTime((UInt16)(dosTime >> 16), (UInt16)(dosTime & 0xFFFF), &fileTime));
}

static const UInt32 kHighDosTime = 0xFF9FBF7D;
static const UInt32 kLowDosTime = 0x210000;

bool FileTimeToDosTime(const FILETIME &fileTime, UInt32 &dosTime)
{
  WORD datePart, timePart;
  if (!::FileTimeToDosDateTime(&fileTime, &datePart, &timePart))
  {
    dosTime = (fileTime.dwHighDateTime >= 0x01C00000) ? kHighDosTime : kLowDosTime;
    return false;
  }
  dosTime = (((UInt32)datePart) << 16) + timePart;
  return true;
}

static const UInt32 kNumTimeQuantumsInSecond = 10000000;
static const UInt64 kUnixTimeStartValue = ((UInt64)kNumTimeQuantumsInSecond) * 60 * 60 * 24 * 134774;

void UnixTimeToFileTime(UInt32 unixTime, FILETIME &fileTime)
{
  UInt64 v = kUnixTimeStartValue + ((UInt64)unixTime) * kNumTimeQuantumsInSecond;
  fileTime.dwLowDateTime = (DWORD)v;
  fileTime.dwHighDateTime = (DWORD)(v >> 32);
}

bool FileTimeToUnixTime(const FILETIME &fileTime, UInt32 &unixTime)
{
  UInt64 winTime = (((UInt64)fileTime.dwHighDateTime) << 32) + fileTime.dwLowDateTime;
  if (winTime < kUnixTimeStartValue)
  {
    unixTime = 0;
    return false;
  }
  winTime = (winTime - kUnixTimeStartValue) / kNumTimeQuantumsInSecond;
  if (winTime > 0xFFFFFFFF)
  {
    unixTime = 0xFFFFFFFF;
    return false;
  }
  unixTime = (UInt32)winTime;
  return true;
}

bool GetSecondsSince1601(unsigned year, unsigned month, unsigned day,
  unsigned hour, unsigned min, unsigned sec, UInt64 &resSeconds)
{
  resSeconds = 0;
  if (year < 1601 || year >= 10000 || month < 1 || month > 12 ||
      day < 1 || day > 31 || hour > 23 || min > 59 || sec > 59)
    return false;
  UInt32 numYears = year - 1601;
  UInt32 numDays = numYears * 365 + numYears / 4 - numYears / 100 + numYears / 400;
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  month--;
  for (unsigned i = 0; i < month; i++)
    numDays += ms[i];
  numDays += day - 1;
  resSeconds = ((UInt64)(numDays * 24 + hour) * 60 + min) * 60 + sec;
  return true;
}

void GetCurUtcFileTime(FILETIME &ft)
{
  SYSTEMTIME st;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
}

}}
