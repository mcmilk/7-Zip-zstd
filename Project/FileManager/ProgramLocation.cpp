// ProgramLocation.h

#include "StdAfx.h"

#include "ProgramLocation.h"

#include "Windows/Registry.h"
#include "Windows/FileName.h"

using namespace NWindows;
using namespace NRegistry;

static LPCTSTR kLMBasePath = _T("Software\\7-ZIP");
static LPCTSTR kAppPathPathKeyValueName2 = _T("Path");

bool GetProgramFolderPath(CSysString &folder)
{
  folder.Empty();
  CKey key;
  if (key.Open(HKEY_LOCAL_MACHINE, kLMBasePath) != ERROR_SUCCESS)
    return false;
  if (key.QueryValue(kAppPathPathKeyValueName2, folder) != ERROR_SUCCESS)
    return false;
  NFile::NName::NormalizeDirPathPrefix(folder);
  return true;
}

/*
bool GetProgramDirPrefix(CSysString &folder)
{
  folder.Empty();
  CKey aKey;
  CSysString aKeyPath = REGSTR_PATH_APPPATHS;
  aKeyPath += kKeyNameDelimiter;
  aKeyPath += kAppPathProgramName;
  if (aKey.Open(HKEY_LOCAL_MACHINE, aKeyPath, KEY_READ) != ERROR_SUCCESS)
    return false;
  if (aKey.QueryValue(kAppPathPathKeyValueName, folder) != ERROR_SUCCESS)
    return false;
  NFile::NName::NormalizeDirPathPrefix(folder);
  return true;
}
*/
