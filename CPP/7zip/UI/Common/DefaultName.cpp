// DefaultName.cpp

#include "StdAfx.h"

#include "DefaultName.h"

static UString GetDefaultName3(const UString &fileName,
    const UString &extension, const UString &addSubExtension)
{
  int extLength = extension.Len();
  int fileNameLength = fileName.Len();
  if (fileNameLength > extLength + 1)
  {
    int dotPos = fileNameLength - (extLength + 1);
    if (fileName[dotPos] == '.')
      if (extension.IsEqualToNoCase(fileName.Ptr(dotPos + 1)))
        return fileName.Left(dotPos) + addSubExtension;
  }
  int dotPos = fileName.ReverseFind(L'.');
  if (dotPos > 0)
    return fileName.Left(dotPos) + addSubExtension;

  if (addSubExtension.IsEmpty())
    return fileName + L"~";
  else
    return fileName + addSubExtension;
}

UString GetDefaultName2(const UString &fileName,
    const UString &extension, const UString &addSubExtension)
{
  UString name = GetDefaultName3(fileName, extension, addSubExtension);
  name.TrimRight();
  return name;
}
