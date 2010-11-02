// ProgramLocation.cpp

#include "StdAfx.h"

#include "../../../../C/Types.h"

#include "ProgramLocation.h"

#include "Windows/DLL.h"

using namespace NWindows;

extern HINSTANCE g_hInstance;

bool GetProgramFolderPath(UString &folder)
{
  if (!NDLL::MyGetModuleFileName(g_hInstance, folder))
    return false;
  int pos = folder.ReverseFind(WCHAR_PATH_SEPARATOR);
  if (pos < 0)
    return false;
  folder = folder.Left(pos + 1);
  return true;
}
