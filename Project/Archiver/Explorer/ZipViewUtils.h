// ZipViewUtils.h

#pragma once

#ifndef __ZIPVIEWUTILS_H
#define __ZIPVIEWUTILS_H

#include "Common/String.h"

class CShellBrowserDisabler
{
  LPSHELLBROWSER m_ShellBrowser;
  HWND GetWindowHandle()
  {
    HWND aHandle;
    if(m_ShellBrowser->GetWindow(&aHandle) != S_OK)
      throw 112059;
    return aHandle;
  }
public:
  CShellBrowserDisabler(LPSHELLBROWSER aShellBrowser): m_ShellBrowser(aShellBrowser)
  {
    ::EnableWindow(GetWindowHandle(), FALSE);
    m_ShellBrowser->EnableModelessSB(FALSE);
  }
  ~CShellBrowserDisabler() 
  { 
    ::EnableWindow(GetWindowHandle(), TRUE);
    m_ShellBrowser->EnableModelessSB(TRUE);
  }
};

#endif


