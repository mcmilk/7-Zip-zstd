// HelpUtils.cpp

#include "StdAfx.h"

#include <HtmlHelp.h>

#include "HelpUtils.h"
#include "ProgramLocation.h"

static LPCTSTR kHelpFileName = _T("7-zip.chm::/");


void ShowHelpWindow(HWND hwnd, LPCTSTR topicFile)
{
  CSysString path;
  if (!::GetProgramFolderPath(path))
  {
    // AfxMessageBox(_T("App Path Registry Item not found"));
    return;
  }
  path += kHelpFileName;
  path += topicFile;
  HtmlHelp(hwnd, path, HH_DISPLAY_TOPIC, NULL);
}


