// ExtractAutoRename.cpp

#include "StdAfx.h"
#include "ExtractAutoRename.h"

#include "Common/Defs.h"
#include "Windows/FileName.h"
#include "Windows/FileFind.h"

using namespace NWindows;

static bool MakeAutoName(const CSysString &aName, const CSysString &anExtension, 
    int aValue, CSysString &aPath)
{
  TCHAR aNumber[32];
  _itot(aValue, aNumber, 10);
  aPath = aName;
  aPath += aNumber;
  aPath += anExtension;
  return NFile::NFind::DoesFileExist(aPath);
}

bool AutoRenamePath(CSysString &aFullProcessedPath)
{
  CSysString aPath;
  int aPosDot = aFullProcessedPath.ReverseFind(TEXT('.'));
  int aSlashDot1 = aFullProcessedPath.ReverseFind(TEXT('\\'));
  int aSlashDot2 = aFullProcessedPath.ReverseFind(TEXT('/'));
  int aSlashDot = MyMin(aSlashDot1, aSlashDot2);
  CSysString aName, anExtension;
  if (aPosDot > aSlashDot &&  aPosDot > 0)
  {
    aName = aFullProcessedPath.Left(aPosDot);
    anExtension = aFullProcessedPath.Mid(aPosDot);
  }
  else
    aName = aFullProcessedPath;
  aName += TEXT('_');
  int anIndexLeft = 1, anIndexRight = (1 << 30);
  while (anIndexLeft != anIndexRight)
  {
    int anIndexMid = (anIndexLeft + anIndexRight) / 2;
    if (MakeAutoName(aName, anExtension, anIndexMid, aPath))
      anIndexLeft = anIndexMid + 1;
    else
      anIndexRight = anIndexMid;
  }
  if (MakeAutoName(aName, anExtension, anIndexRight, aFullProcessedPath))
    return false;
  return true;
}
