// PropIDUtils.cpp

#include "StdAfx.h"

#include "PropIDUtils.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/FileFind.h"
#include "Windows/PropVariantConversions.h"

#include "../../PropID.h"

using namespace NWindows;

static UString ConvertUInt32ToString(UInt32 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

static void ConvertUInt32ToHex(UInt32 value, wchar_t *s)
{
  for (int i = 0; i < 8; i++)
  {
    int t = value & 0xF;
    value >>= 4;
    s[7 - i] = (wchar_t)((t < 10) ? (L'0' + t) : (L'A' + (t - 10)));
  }
  s[8] = L'\0';
}

UString ConvertPropertyToString(const PROPVARIANT &propVariant, PROPID propID, bool full)
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
      return ConvertFileTimeToString(localFileTime, true, full);
    }
    case kpidCRC:
    {
      if(propVariant.vt != VT_UI4)
        break;
      wchar_t temp[12];
      ConvertUInt32ToHex(propVariant.ulVal, temp);
      return temp;
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
        return ConvertUInt32ToString(size >> 20) + L"MB";
      if (size % (1 << 10) == 0)
        return ConvertUInt32ToString(size >> 10) + L"KB";
      return ConvertUInt32ToString(size);
    }
  }
  return ConvertPropVariantToString(propVariant);
}
