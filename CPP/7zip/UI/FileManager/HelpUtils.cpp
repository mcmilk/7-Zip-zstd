// HelpUtils.cpp

#include "StdAfx.h"

#include <HtmlHelp.h>

#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"

#include "HelpUtils.h"

static LPCWSTR kHelpFileName = L"7-zip.chm::/";

#ifdef UNDER_CE
void ShowHelpWindow(HWND, LPCWSTR)
{
}
#else
void ShowHelpWindow(HWND hwnd, LPCWSTR topicFile)
{
  FString path = NWindows::NDLL::GetModuleDirPrefix();
  HtmlHelp(hwnd, GetSystemString(fs2us(path) + kHelpFileName + topicFile), HH_DISPLAY_TOPIC, NULL);
}
#endif
