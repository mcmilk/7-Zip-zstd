// ArchiveName.cpp

#include "StdAfx.h"

#include "../../../Windows/FileDir.h"

#include "ExtractingFilePath.h"
#include "ArchiveName.h"

using namespace NWindows;

UString CreateArchiveName(const NFile::NFind::CFileInfo fileInfo, bool keepName)
{
  FString resultName = fileInfo.Name;
  if (!fileInfo.IsDir() && !keepName)
  {
    int dotPos = resultName.ReverseFind(FTEXT('.'));
    if (dotPos > 0)
    {
      FString archiveName2 = resultName.Left(dotPos);
      if (archiveName2.ReverseFind(FTEXT('.')) < 0)
        resultName = archiveName2;
    }
  }
  return GetCorrectFsPath(fs2us(resultName));
}

static FString CreateArchiveName2(const FString &srcName, bool fromPrev, bool keepName)
{
  FString resultName = FTEXT("Archive");
  if (fromPrev)
  {
    FString dirPrefix;
    if (NFile::NDir::GetOnlyDirPrefix(srcName, dirPrefix))
    {
      if (dirPrefix.Len() > 0)
        if (dirPrefix.Back() == FCHAR_PATH_SEPARATOR)
        {
          dirPrefix.DeleteBack();
          NFile::NFind::CFileInfo fileInfo;
          if (fileInfo.Find(dirPrefix))
            resultName = fileInfo.Name;
        }
    }
  }
  else
  {
    NFile::NFind::CFileInfo fileInfo;
    if (!fileInfo.Find(srcName))
      // return resultName;
      return srcName;
    resultName = fileInfo.Name;
    if (!fileInfo.IsDir() && !keepName)
    {
      int dotPos = resultName.ReverseFind('.');
      if (dotPos > 0)
      {
        FString archiveName2 = resultName.Left(dotPos);
        if (archiveName2.ReverseFind(FTEXT('.')) < 0)
          resultName = archiveName2;
      }
    }
  }
  return resultName;
}

UString CreateArchiveName(const UString &srcName, bool fromPrev, bool keepName)
{
  return GetCorrectFsPath(fs2us(CreateArchiveName2(us2fs(srcName), fromPrev, keepName)));
}
