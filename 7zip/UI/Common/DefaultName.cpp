// DefaultName.cpp

#include "StdAfx.h"

#include "Windows/FileDir.h"

#include "Common/StringConvert.h"
#include "DefaultName.h"

const wchar_t *kEmptyFileAlias = L"[Content]";

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

UString GetDefaultName(const UString &fullFileName, 
    const UString &extension, const UString &addSubExtension)
{
  UString fileName;
  if (!GetOnlyName(fullFileName, fileName))
    throw 5011749;
  int extLength = extension.Length();
  int fileNameLength = fileName.Length();
  if (fileNameLength <= extLength + 1)
    return kEmptyFileAlias;
  int dotPos = fileNameLength - (extLength + 1);
  if (fileName[dotPos] != '.')
    return kEmptyFileAlias;
  if (extension.CollateNoCase(fileName.Mid(dotPos + 1)) == 0)
    return fileName.Left(dotPos) + addSubExtension;
  return kEmptyFileAlias;
}

