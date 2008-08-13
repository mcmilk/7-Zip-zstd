// Archive/IsoItem.h

#ifndef __ARCHIVE_ISO_ITEM_H
#define __ARCHIVE_ISO_ITEM_H

#include "Common/Types.h"
#include "Common/MyString.h"
#include "Common/Buffer.h"

#include "Windows/Time.h"

#include "IsoHeader.h"

namespace NArchive {
namespace NIso {

struct CRecordingDateTime
{
  Byte Year;
  Byte Month;
  Byte Day;
  Byte Hour;
  Byte Minute;
  Byte Second;
  signed char GmtOffset; // min intervals from -48 (West) to +52 (East) recorded.
  
  bool GetFileTime(FILETIME &ft) const
  {
    UInt64 value;
    bool res = NWindows::NTime::GetSecondsSince1601(Year + 1900, Month, Day, Hour, Minute, Second, value);
    if (res)
    {
      value -= (UInt64)((Int64)GmtOffset * 15 * 60);
      value *= 10000000;
    }
    ft.dwLowDateTime = (DWORD)value;
    ft.dwHighDateTime = (DWORD)(value >> 32);
    return res;
  }
};

struct CDirRecord
{
  Byte ExtendedAttributeRecordLen;
  UInt32 ExtentLocation;
  UInt32 DataLength;
  CRecordingDateTime DateTime;
  Byte FileFlags;
  Byte FileUnitSize;
  Byte InterleaveGapSize;
  UInt16 VolSequenceNumber;
  CByteBuffer FileId;
  CByteBuffer SystemUse;

  bool IsDir() const { return  (FileFlags & NFileFlags::kDirectory) != 0; }
  bool IsSystemItem() const
  {
    if (FileId.GetCapacity() != 1)
      return false;
    Byte b = *(const Byte *)FileId;
    return (b == 0 || b == 1);
  }

  const Byte* FindSuspName(int skipSize, int &lenRes) const
  {
    lenRes = 0;
    const Byte *p = (const Byte *)SystemUse + skipSize;
    int length = (int)(SystemUse.GetCapacity() - skipSize);
    while (length >= 5)
    {
      int len = p[2];
      if (p[0] == 'N' && p[1] == 'M' && p[3] == 1)
      {
        lenRes = len - 5;
        return p + 5;
      }
      p += len;
      length -= len;
    }
    return 0;
  }

  int GetLengthCur(bool checkSusp, int skipSize) const
  {
    if (checkSusp)
    {
      int len;
      const Byte *res = FindSuspName(skipSize, len);
      if (res != 0)
        return len;
    }
    return (int)FileId.GetCapacity();
  }

  const Byte* GetNameCur(bool checkSusp, int skipSize) const
  {
    if (checkSusp)
    {
      int len;
      const Byte *res = FindSuspName(skipSize, len);
      if (res != 0)
        return res;
    }
    return (const Byte *)FileId;
  }


  bool CheckSusp(const Byte *p, int &startPos) const
  {
    if (p[0] == 'S' &&
        p[1] == 'P' &&
        p[2] == 0x7 &&
        p[3] == 0x1 &&
        p[4] == 0xBE &&
        p[5] == 0xEF)
    {
      startPos = p[6];
      return true;
    }
    return false;
  }

  bool CheckSusp(int &startPos) const
  {
    const Byte *p = (const Byte *)SystemUse;
    int length = (int)SystemUse.GetCapacity();
    const int kMinLen = 7;
    if (length < kMinLen)
      return false;
    if (CheckSusp(p, startPos))
      return true;
    const int kOffset2 = 14;
    if (length < kOffset2 + kMinLen)
      return false;
    return CheckSusp(p + kOffset2, startPos);
  }
};

}}

#endif
