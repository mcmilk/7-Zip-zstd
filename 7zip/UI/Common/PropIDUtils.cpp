// PropIDUtils.cpp

#include "StdAfx.h"

#include "PropIDUtils.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/FileFind.h"
#include "Windows/PropVariantConversions.h"

#include "../../PropID.h"

using namespace NWindows;

static UString ConvertUINT32ToString(UInt32 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

UString ConvertPropertyToString(const PROPVARIANT &propVariant, 
    PROPID propID, bool full)
{
  switch(propID)
  {
    case kpidCreationTime:
    case kpidLastWriteTime:
    case kpidLastAccessTime:
    {
      if (propVariant.vt != VT_FILETIME)
        return UString(); // It is error;
      FILETIME localFileTime;
      if (propVariant.filetime.dwHighDateTime == 0 && 
          propVariant.filetime.dwLowDateTime == 0)
        return UString();
      if (!::FileTimeToLocalFileTime(&propVariant.filetime, &localFileTime))
        return UString(); // It is error;
      return ConvertFileTimeToString2(localFileTime, true, full);
    }
    case kpidCRC:
    {
      if(propVariant.vt != VT_UI4)
        break;
      TCHAR temp[17];
      wsprintf(temp, TEXT("%08X"), propVariant.ulVal);
      return GetUnicodeString(temp);
    }
    case kpidAttributes:
    {
      if(propVariant.vt != VT_UI4)
        break;
      UString result;
      UInt32 attributes = propVariant.ulVal;
      if (NFile::NFind::NAttributes::IsReadOnly(attributes)) result += L'R';
      if (NFile::NFind::NAttributes::IsHidden(attributes)) result += L'H';
      if (NFile::NFind::NAttributes::IsSystem(attributes)) result += L'S';
      if (NFile::NFind::NAttributes::IsDirectory(attributes)) result += L'D';
      if (NFile::NFind::NAttributes::IsArchived(attributes)) result += L'A';
      if (NFile::NFind::NAttributes::IsCompressed(attributes)) result += L'C';
      if (NFile::NFind::NAttributes::IsEncrypted(attributes)) result += L'E';
      return result;
    }
    case kpidDictionarySize:
    {
      if(propVariant.vt != VT_UI4)
        break;
      UInt32 size = propVariant.ulVal;
      if (size % (1 << 20) == 0)
        return ConvertUINT32ToString(size >> 20) + L"MB";
      if (size % (1 << 10) == 0)
        return ConvertUINT32ToString(size >> 10) + L"KB";
      return ConvertUINT32ToString(size);
    }
  }
  return ConvertPropVariantToString(propVariant);
}
