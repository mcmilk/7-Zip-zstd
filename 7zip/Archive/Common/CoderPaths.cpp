// CodersPath.cpp

#include "StdAfx.h"
#include "../../../Common/String.h"

extern HINSTANCE g_hInstance;

static CSysString GetLibraryPath()
{
  TCHAR fullPath[MAX_PATH + 1];
  ::GetModuleFileName(g_hInstance, fullPath, MAX_PATH);
  return fullPath;
}

static CSysString GetLibraryFolderPrefix()
{
  CSysString path = GetLibraryPath();
  int pos = path.ReverseFind(TEXT('\\'));
  return path.Left(pos + 1);
}

CSysString GetBaseFolderPrefix()
{
  CSysString libPrefix = GetLibraryFolderPrefix();
  CSysString temp = libPrefix;
  temp.Delete(temp.Length() - 1);
  int pos = temp.ReverseFind(TEXT('\\'));
  return temp.Left(pos + 1);
}

CSysString GetCodecsFolderPrefix()
{
  return GetBaseFolderPrefix() + TEXT("Compress\\");
}
