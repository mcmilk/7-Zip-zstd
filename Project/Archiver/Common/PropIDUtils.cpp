// PropIDUtils.cpp

#include "StdAfx.h"

#include "PropIDUtils.h"

#include "Windows/FileFind.h"

#include "../Format/Common/IArchiveHandler.h"
#include "Windows/PropVariantConversions.h"

using namespace NWindows;

static CSysString ConvertUINT32ToString(UINT32 aValue)
{
  TCHAR aBuffer[16];
  return _ultot(aValue, aBuffer, 10);
}

CSysString ConvertPropertyToString(const PROPVARIANT &aPropVariant, PROPID aPropID)
{
  switch(aPropID)
  {
    case kaipidCreationTime:
    case kaipidLastWriteTime:
    case kaipidLastAccessTime:
    {
      if (aPropVariant.vt != VT_FILETIME)
        return CSysString(); // It is error;
      FILETIME aLocalFileTime;
      if (!::FileTimeToLocalFileTime(&aPropVariant.filetime, &aLocalFileTime))
        return CSysString(); // It is error;
      return ConvertFileTimeToString(aLocalFileTime);
    }
    case kaipidCRC:
    {
      if(aPropVariant.vt != VT_UI4)
        break;
      TCHAR aTmp[17];
      _stprintf(aTmp, _T("%08X"), aPropVariant.ulVal);
      return aTmp;
    }
    case kaipidAttributes:
    {
      if(aPropVariant.vt != VT_UI4)
        break;
      CSysString aResult;
      UINT32 anAttributes = aPropVariant.ulVal;
      if (NFile::NFind::NAttributes::IsReadOnly(anAttributes)) aResult += _T('R');
      if (NFile::NFind::NAttributes::IsHidden(anAttributes)) aResult += _T('H');
      if (NFile::NFind::NAttributes::IsSystem(anAttributes)) aResult += _T('S');
      if (NFile::NFind::NAttributes::IsDirectory(anAttributes)) aResult += _T('D');
      if (NFile::NFind::NAttributes::IsArchived(anAttributes)) aResult += _T('A');
      if (NFile::NFind::NAttributes::IsCompressed(anAttributes)) aResult += _T('C');
      if (NFile::NFind::NAttributes::IsEncrypted(anAttributes)) aResult += _T('E');
      return aResult;
    }
    case kaipidDictionarySize:
    {
      if(aPropVariant.vt != VT_UI4)
        break;
      UINT32 aSize = aPropVariant.ulVal;
      if (aSize % (1 << 20) == 0)
        return ConvertUINT32ToString(aSize >> 20) + _T("MB");
      if (aSize % (1 << 10) == 0)
        return ConvertUINT32ToString(aSize >> 10) + _T("KB");
      return ConvertUINT32ToString(aSize);
    }
  }
  return ConvertPropVariantToString(aPropVariant);
}
