// HelpUtils.cpp

#include "StdAfx.h"

#include "HelpUtils.h"

#if defined(UNDER_CE) || !defined(_WIN32)

void ShowHelpWindow(HWND, LPCWSTR)
{
}

#else

#include <HtmlHelp.h>

#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"

static LPCWSTR kHelpFileName = L"7-zip.chm::/";

void ShowHelpWindow(HWND hwnd, LPCWSTR topicFile)
{
  FString path = NWindows::NDLL::GetModuleDirPrefix();
  HtmlHelp(hwnd, GetSystemString(fs2us(path) + kHelpFileName + topicFile), HH_DISPLAY_TOPIC, 0);
}

#endif
