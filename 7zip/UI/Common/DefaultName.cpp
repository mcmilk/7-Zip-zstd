// DefaultName.cpp

#include "StdAfx.h"

#include "Windows/FileDir.h"

#include "Common/StringConvert.h"
#include "DefaultName.h"

const wchar_t *kEmptyFileAlias = L"[Content]";

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

UString GetDefaultName(const CSysString &fullFileName, 
    const UString &extension, const UString &addSubExtension)
{
  CSysString fileNameSys;
  if (!GetOnlyName(fullFileName, fileNameSys))
    throw 5011749;
  UString fileName = GetUnicodeString(fileNameSys, 
        (AreFileApisANSI() ? CP_ACP : CP_OEMCP));
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

