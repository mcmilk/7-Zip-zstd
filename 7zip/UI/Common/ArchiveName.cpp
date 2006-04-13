// ArchiveName.cpp

#include "StdAfx.h"

#include "Windows/FileFind.h"
#include "Windows/FileDir.h"

using namespace NWindows;

UString CreateArchiveName(const UString &srcName, bool fromPrev, bool keepName)
{
  UString resultName = L"Archive";
  if (fromPrev)
  {
    UString dirPrefix;
    if (NFile::NDirectory::GetOnlyDirPrefix(srcName, dirPrefix))
    {
      if (dirPrefix.Length() > 0)
        if (dirPrefix[dirPrefix.Length() - 1] == '\\')
        {
          dirPrefix.Delete(dirPrefix.Length() - 1);
          NFile::NFind::CFileInfoW fileInfo;
          if (NFile::NFind::FindFile(dirPrefix, fileInfo))
            resultName = fileInfo.Name;
        }
    }
  }
  else
  {
    NFile::NFind::CFileInfoW fileInfo;
    if (!NFile::NFind::FindFile(srcName, fileInfo))
      return resultName;
    resultName = fileInfo.Name;
    if (!fileInfo.IsDirectory() && !keepName)
    {
      int dotPos = resultName.ReverseFind('.');
      if (dotPos > 0)
      {
        UString archiveName2 = resultName.Left(dotPos);
        if (archiveName2.ReverseFind('.') < 0)
          resultName = archiveName2;
      }
    }
  }
  return resultName;
}
