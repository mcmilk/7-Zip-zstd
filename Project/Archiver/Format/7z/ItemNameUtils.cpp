// 7z/ItemNameUtils.cpp

#include "StdAfx.h"

#include "ItemNameUtils.h"

#include "Windows/FileName.h"

namespace NArchive {
namespace N7z {
namespace NItemName {

static const char kIllegalDirDelimiter = '\\';
static const char kDirDelimiter = '/';

UString MakeLegalName(const UString &aName)
{
  UString aLegalName = aName;
  aLegalName.Replace(kIllegalDirDelimiter, kDirDelimiter);
  return aLegalName;
}

UString GetOSName(const UString &anInternalName)
{
  UString aNewName = anInternalName;
  aNewName.Replace(kDirDelimiter, NWindows::NFile::NName::kDirDelimiter);
  return aNewName;
}


}}}
