// DefaultName.cpp

#include "StdAfx.h"

#include "Windows/FileDir.h"

#include "Common/StringConvert.h"
#include "DefaultName.h"

const wchar_t *kEmptyFileAlias = L"[Content]";

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

UString GetDefaultName(const CSysString &aFullFileName, 
    const CSysString &anExtension, const UString &anAddSubExtension)
{
  CSysString aFileName;
  if (!GetOnlyName(aFullFileName, aFileName))
    throw 5011749;
  int anExtLength = anExtension.Length();
  int aFileNameLength = aFileName.Length();
  if (aFileNameLength <= anExtLength + 1)
    return kEmptyFileAlias;
  int aDotPos = aFileNameLength - (anExtLength + 1);
  if (aFileName[aDotPos] != '.')
    return kEmptyFileAlias;
  if (anExtension.CollateNoCase(aFileName.Mid(aDotPos + 1)) == 0)
    return GetUnicodeString(aFileName.Left(aDotPos)) + anAddSubExtension;
  return kEmptyFileAlias;
}

