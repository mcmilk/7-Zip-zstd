// Archive/Common/ItemNameUtils.cpp

#include "StdAfx.h"

#include "ItemNameUtils.h"

namespace NArchive {
namespace NItemName {

static const wchar_t kOSDirDelimiter = '\\';
static const wchar_t kDirDelimiter = '/';

UString MakeLegalName(const UString &aName)
{
  UString aZipName = aName;
  aZipName.Replace(kOSDirDelimiter, kDirDelimiter);
  return aZipName;
}

UString GetOSName(const UString &aName)
{
  UString aNewName = aName;
  aNewName.Replace(kDirDelimiter, kOSDirDelimiter);
  return aNewName;
}

UString GetOSName2(const UString &aName)
{
  if (aName.IsEmpty())
    return UString();
  UString aNewName = GetOSName(aName);
  if (aNewName[aNewName.Length() - 1] == kOSDirDelimiter)
    aNewName.Delete(aNewName.Length() - 1);
  return aNewName;
}

}}
