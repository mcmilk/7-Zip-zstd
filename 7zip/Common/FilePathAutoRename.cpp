// FilePathAutoRename.cpp

#include "StdAfx.h"
#include "FilePathAutoRename.h"

#include "Common/Defs.h"
#include "Common/IntToString.h"

#include "Windows/FileName.h"
#include "Windows/FileFind.h"

using namespace NWindows;

static bool MakeAutoName(const CSysString &name, const CSysString &extension, 
    int value, CSysString &path)
{
  TCHAR number[32];
  ConvertUINT64ToString(value, number);
  path = name;
  path += number;
  path += extension;
  return NFile::NFind::DoesFileExist(path);
}

bool AutoRenamePath(CSysString &fullProcessedPath)
{
  CSysString path;
  int dotPos = fullProcessedPath.ReverseFind(TEXT('.'));
  int slashDot1 = fullProcessedPath.ReverseFind(TEXT('\\'));
  int slashDot2 = fullProcessedPath.ReverseFind(TEXT('/'));
  int slashDot = MyMin(slashDot1, slashDot2);
  CSysString name, extension;
  if (dotPos > slashDot &&  dotPos > 0)
  {
    name = fullProcessedPath.Left(dotPos);
    extension = fullProcessedPath.Mid(dotPos);
  }
  else
    name = fullProcessedPath;
  name += TEXT('_');
  int indexLeft = 1, indexRight = (1 << 30);
  while (indexLeft != indexRight)
  {
    int indexMid = (indexLeft + indexRight) / 2;
    if (MakeAutoName(name, extension, indexMid, path))
      indexLeft = indexMid + 1;
    else
      indexRight = indexMid;
  }
  if (MakeAutoName(name, extension, indexRight, fullProcessedPath))
    return false;
  return true;
}
