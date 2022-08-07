// HelpUtils.cpp

#include "StdAfx.h"

#include "HelpUtils.h"

#if defined(UNDER_CE) || !defined(_WIN32) /* || !defined(_MSC_VER) */

void ShowHelpWindow(LPCSTR)
{
}

#else

// #define USE_EXTERNAL_HELP

#if defined(_MSC_VER)
#endif

#ifdef USE_EXTERNAL_HELP

#include "../../../Windows/ProcessUtils.h"
#include "../../../Windows/FileDir.h"
#include "../../../Windows/FileName.h"

#else
#include <HtmlHelp.h>
#endif

#include "../../../Common/StringConvert.h"

#include "../../../Windows/DLL.h"

#define kHelpFileName "7-zip.chm::/"

void ShowHelpWindow(LPCSTR topicFile)
{
  FString path = NWindows::NDLL::GetModuleDirPrefix();
  path += kHelpFileName;
  path += topicFile;
 #ifdef USE_EXTERNAL_HELP
  FString prog;

  #ifdef UNDER_CE
    prog = "\\Windows\\";
  #else
    if (!NWindows::NFile::NDir::GetWindowsDir(prog))
      return;
    NWindows::NFile::NName::NormalizeDirPathPrefix(prog);
  #endif
  prog += "hh.exe";

  UString params;
  params += '"';
  params += fs2us(path);
  params += '"';

  NWindows::CProcess process;
  const WRes wres = process.Create(fs2us(prog), params, NULL); // curDir);
  if (wres != 0)
  {
    /*
    HRESULT hres = HRESULT_FROM_WIN32(wres);
    ErrorMessageHRESULT(hres, imageName);
    return hres;
    */
  }
 #else
  // HWND hwnd = NULL;
  HtmlHelp(NULL, GetSystemString(fs2us(path)), HH_DISPLAY_TOPIC, 0);
 #endif
}

#endif
