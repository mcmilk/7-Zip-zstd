// ProgramLocation.h

#include "StdAfx.h"

#include "ProgramLocation.h"

#include "Windows/FileName.h"

using namespace NWindows;

extern HINSTANCE g_hInstance;

bool GetProgramFolderPath(CSysString &folder)
{
  folder.Empty();
  TCHAR fullPath[MAX_PATH + 1];
  ::GetModuleFileName(g_hInstance, fullPath, MAX_PATH);
  CSysString path = fullPath;
  int pos = path.ReverseFind(TEXT('\\'));
  if (pos < 0)
    return false;
  folder = path.Left(pos + 1);
  return true;
}

