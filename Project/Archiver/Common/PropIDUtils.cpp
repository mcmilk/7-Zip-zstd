// PropIDUtils.cpp

#include "StdAfx.h"

#include "PropIDUtils.h"

#include "Common/IntToString.h"

#include "Windows/FileFind.h"
#include "Windows/PropVariantConversions.h"

#include "Interface/PropID.h"

using namespace NWindows;

static CSysString ConvertUINT32ToString(UINT32 value)
{
  TCHAR buffer[32];
  ConvertUINT64ToString(value, buffer);
  return buffer;
}

CSysString ConvertPropertyToString(const PROPVARIANT &propVariant, PROPID aPropID,
    bool full)
{
  switch(aPropID)
  {
    case kpidCreationTime:
    case kpidLastWriteTime:
    case kpidLastAccessTime:
    {
      if (propVariant.vt != VT_FILETIME)
        return CSysString(); // It is error;
      FILETIME aLocalFileTime;
      if (propVariant.filetime.dwHighDateTime == 0 && 
          propVariant.filetime.dwLowDateTime == 0)
        return CSysString();
      if (!::FileTimeToLocalFileTime(&propVariant.filetime, &aLocalFileTime))
        return CSysString(); // It is error;
      return ConvertFileTimeToString2(aLocalFileTime, true, full);
    }
    case kpidCRC:
    {
      if(propVariant.vt != VT_UI4)
        break;
      TCHAR temp[17];
      wsprintf(temp, _T("%08X"), propVariant.ulVal);
      return temp;
    }
    case kpidAttributes:
    {
      if(propVariant.vt != VT_UI4)
        break;
      CSysString result;
      UINT32 attributes = propVariant.ulVal;
      if (NFile::NFind::NAttributes::IsReadOnly(attributes)) result += _T('R');
      if (NFile::NFind::NAttributes::IsHidden(attributes)) result += _T('H');
      if (NFile::NFind::NAttributes::IsSystem(attributes)) result += _T('S');
      if (NFile::NFind::NAttributes::IsDirectory(attributes)) result += _T('D');
      if (NFile::NFind::NAttributes::IsArchived(attributes)) result += _T('A');
      if (NFile::NFind::NAttributes::IsCompressed(attributes)) result += _T('C');
      if (NFile::NFind::NAttributes::IsEncrypted(attributes)) result += _T('E');
      return result;
    }
    case kpidDictionarySize:
    {
      if(propVariant.vt != VT_UI4)
        break;
      UINT32 aSize = propVariant.ulVal;
      if (aSize % (1 << 20) == 0)
        return ConvertUINT32ToString(aSize >> 20) + _T("MB");
      if (aSize % (1 << 10) == 0)
        return ConvertUINT32ToString(aSize >> 10) + _T("KB");
      return ConvertUINT32ToString(aSize);
    }
  }
  return ConvertPropVariantToString(propVariant);
}
