// Archive/Tar/ItemNameUtils.cpp

#include "StdAfx.h"

#include "Archive/Tar/ItemNameUtils.h"
#include "Windows/FileName.h"

namespace NArchive {
namespace NTar {
namespace NItemName {

static const char kIllegalDirDelimiter = '\\';
static const char kDirDelimiter = '/';

bool IsNameLegal(const UString &aName)
{
  return (aName.Find(kIllegalDirDelimiter) < 0); // it's not full test;
}

bool IsItDirName(const AString &aName)
{
  if (aName.IsEmpty())
    return false;
  // return (aName.ReverseFind(kDirDelimiter) == aName.Length() - 1);
  return (aName[aName.Length() - 1] == kDirDelimiter);
}

AString MakeLegalName(const AString &aName)
{
  AString aZipName = aName;
  aZipName.Replace(kIllegalDirDelimiter, kDirDelimiter);
  return aZipName;
}

AString MakeLegalDirName(const AString &aDirName)
{
  AString aZipDirName = MakeLegalName(aDirName);
  if (aZipDirName.IsEmpty())
    throw 12201516;
  // if (aZipDirName[aZipDirName.Length()-1] != kDirDelimiter)
  if (aZipDirName.ReverseFind(kDirDelimiter) != aZipDirName.Length() - 1)
    aZipDirName += kDirDelimiter;
  return aZipDirName;
}

UString GetOSName(const UString &aZipDirName)
{
  if (aZipDirName.IsEmpty())
    throw 12220115;
  UString aNewName = aZipDirName;
  int aLastIndex = aNewName.Length() - 1;
  if (aNewName[aLastIndex] == kDirDelimiter)
    aNewName.Delete(aLastIndex);
  aNewName.Replace(kDirDelimiter, NWindows::NFile::NName::kDirDelimiter);
  return aNewName;
}

}}}
