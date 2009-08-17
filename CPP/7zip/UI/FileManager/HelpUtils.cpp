// HelpUtils.cpp

#include "StdAfx.h"

#include <HtmlHelp.h>

#include "Common/StringConvert.h"
#include "HelpUtils.h"
#include "ProgramLocation.h"

static LPCWSTR kHelpFileName = L"7-zip.chm::/";

#ifdef UNDER_CE
void ShowHelpWindow(HWND, LPCWSTR)
{
}
#else
void ShowHelpWindow(HWND hwnd, LPCWSTR topicFile)
{
  UString path;
  if (!::GetProgramFolderPath(path))
    return;
  path += kHelpFileName;
  path += topicFile;
  HtmlHelp(hwnd, GetSystemString(path), HH_DISPLAY_TOPIC, NULL);
}
#endif
