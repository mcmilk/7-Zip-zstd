// Windows/FileName.cpp

#include "StdAfx.h"

#include "Windows/FileName.h"
#include "Common/WildCard.h"

namespace NWindows {
namespace NFile {
namespace NName {

static const wchar_t kDiskDelimiter = L':';

/*
static bool IsCharAPrefixDelimiter(wchar_t aChar)
{
  return (aChar == kDirDelimiter || aChar == kDiskDelimiter);
}
*/

void NormalizeDirPathPrefix(CSysString &aDirPath)
{
  if (aDirPath.IsEmpty())
    return;
  if (aDirPath.ReverseFind(kDirDelimiter) != aDirPath.Length() - 1)
    aDirPath += kDirDelimiter;
}

namespace NPathType
{
  EEnum GetPathType(const UString &aPath)
  {
    if (aPath.Length() <= 2)
      return kLocal;
    if (aPath[0] == kDirDelimiter && aPath[1] == kDirDelimiter)
      return kUNC;
    return kLocal;
  }
}

void CParsedPath::ParsePath(const UString &aPath)
{
  int aCurPos = 0;
  switch (NPathType::GetPathType(aPath))
  {
    case NPathType::kLocal:
    {
      int aPosDiskDelimiter = aPath.Find(kDiskDelimiter);
      if(aPosDiskDelimiter >= 0)
      {
        aCurPos = aPosDiskDelimiter + 1;
        if (aPath.Length() > aCurPos)
          if(aPath[aCurPos] == kDirDelimiter)
            aCurPos++;
      }
      break;
    }
    case NPathType::kUNC:
    {
      int aCurPos = aPath.Find(kDirDelimiter, 2);
      if(aCurPos < 0)
        aCurPos = aPath.Length();
      else
        aCurPos++;
    }
  }
  Prefix = aPath.Left(aCurPos);
  SplitPathToParts(aPath.Mid(aCurPos), PathParts);
}

UString CParsedPath::MergePath() const
{
  UString aResult = Prefix;
  for(int i = 0; i < PathParts.Size(); i++)
  {
    if (i != 0)
      aResult += kDirDelimiter;
    aResult += PathParts[i];
  }
  return aResult;
}

const TCHAR kExtensionDelimiter = '.';

void SplitNameToPureNameAndExtension(const CSysString &aFullName, CSysString &aPureName,
  CSysString &anExtensionDelimiter, CSysString &anExtension)
{
  int anIndex = aFullName.ReverseFind(kExtensionDelimiter);
  if (anIndex < 0)
  {
    aPureName = aFullName;
    anExtensionDelimiter.Empty();
    anExtension.Empty();
  }
  else
  {
    aPureName = aFullName.Left(anIndex);
    anExtensionDelimiter = kExtensionDelimiter;
    anExtension = aFullName.Mid(anIndex + 1);
  }
}

}}}
