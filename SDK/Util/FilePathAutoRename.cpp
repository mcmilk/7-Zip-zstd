// FilePathAutoRename.cpp

#include "StdAfx.h"
#include "FilePathAutoRename.h"

#include "Common/Defs.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"

using namespace NWindows;

static bool MakeAutoName(const CSysString &name, const CSysString &extension, 
    int value, CSysString &path)
{
  TCHAR number[32];
  _itot(value, number, 10);
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
    int anIndexMid = (indexLeft + indexRight) / 2;
    if (MakeAutoName(name, extension, anIndexMid, path))
      indexLeft = anIndexMid + 1;
    else
      indexRight = anIndexMid;
  }
  if (MakeAutoName(name, extension, indexRight, fullProcessedPath))
    return false;
  return true;
}
