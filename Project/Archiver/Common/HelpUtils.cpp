// HelpUtils.cpp

#include "StdAfx.h"

#include <HtmlHelp.h>

#include "../Common/HelpUtils.h"

#include "../Common/ZipRegistryMain.h"
#include "Windows/Registry.h"
#include "Windows/FileName.h"

using namespace NWindows;
using namespace NRegistry;


static LPCTSTR kHelpFileName = _T("7-zip.chm::/");

static LPCTSTR kAppPathProgramName = _T("7-zipCfg.exe");
static LPCTSTR kAppPathPathKeyValueName = _T("Path");

bool GetProgramDirPrefix(CSysString &aFolder)
{
  CKey aKey;
  CSysString aKeyPath = REGSTR_PATH_APPPATHS;
  aKeyPath += kKeyNameDelimiter;
  aKeyPath += kAppPathProgramName;
  if (aKey.Open(HKEY_LOCAL_MACHINE, aKeyPath, KEY_READ) != ERROR_SUCCESS)
    return false;
  if (aKey.QueryValue(kAppPathPathKeyValueName, aFolder) != ERROR_SUCCESS)
    return false;
  NFile::NName::NormalizeDirPathPrefix(aFolder);
  return true;
}

void ShowHelpWindow(HWND aHWNDForHelp, LPCTSTR aTopicFile)
{
  CSysString aFile;
  if (!::GetProgramDirPrefix(aFile))
  {
    // AfxMessageBox(_T("App Path Registry Item not found"));
    return;
  }
  aFile += kHelpFileName;
  aFile += aTopicFile;
  HtmlHelp(aHWNDForHelp, aFile, HH_DISPLAY_TOPIC, NULL);
}


