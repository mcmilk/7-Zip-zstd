// Archive/IsoItem.h

#ifndef __ARCHIVE_ISO_ITEM_H
#define __ARCHIVE_ISO_ITEM_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Common/Buffer.h"

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
    SYSTEMTIME st;
    st.wYear = Year + 1900;
    st.wMonth = Month;
    st.wDayOfWeek = 0; // check it
    st.wDay = Day;
    st.wHour = Hour;
    st.wMinute = Minute;
    st.wSecond = Second;
    st.wMilliseconds = 0;
    if (!SystemTimeToFileTime(&st, &ft))
      return false;
    UInt64 value =  (((UInt64)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
    value += (UInt64)((Int64)(int)GmtOffset * 15 * 60);
    ft.dwLowDateTime = (DWORD)value;
    ft.dwHighDateTime = DWORD(value >> 32);
    return true;
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
};

}}

#endif
