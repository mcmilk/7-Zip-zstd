// Windows/Window.cpp

#include "StdAfx.h"

#include "Windows/Window.h"
#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

namespace NWindows {

#ifndef _UNICODE
bool CWindow::SetText(LPCWSTR s)
{ 
  if (::SetWindowTextW(_window, s))
    return true;
  if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return false;
  return SetText(UnicodeStringToMultiByte(s));
}
#endif

bool CWindow::GetText(CSysString &s)
{
  s.Empty();
  int length = GetTextLength();
  if (length == 0)
    return (::GetLastError() == ERROR_SUCCESS);
  length = GetText(s.GetBuffer(length), length + 1);
  s.ReleaseBuffer();
  if (length == 0)
    return (::GetLastError() != ERROR_SUCCESS);
  return true;
}

#ifndef _UNICODE
bool CWindow::GetText(UString &s)
{
  s.Empty();
  int length = GetWindowTextLengthW(_window);
  if (length == 0)
  {
    UINT lastError = ::GetLastError();
    if (lastError == ERROR_SUCCESS)
      return true;
    if (lastError != ERROR_CALL_NOT_IMPLEMENTED)
      return false;
    CSysString sysString;
    bool result = GetText(sysString);
    s = GetUnicodeString(sysString);
    return result;
  }
  length = GetWindowTextW(_window, s.GetBuffer(length), length + 1);
  s.ReleaseBuffer();
  if (length == 0)
    return (::GetLastError() == ERROR_SUCCESS);
  return true;
}
#endif
  
/*
bool CWindow::ModifyStyleBase(int styleOffset,
  DWORD remove, DWORD add, UINT flags)
{
  DWORD style = GetWindowLong(styleOffset);
  DWORD newStyle = (style & ~remove) | add;
  if (style == newStyle)
    return false; // it is not good 

  SetWindowLong(styleOffset, newStyle);
  if (flags != 0)
  {
    ::SetWindowPos(_window, NULL, 0, 0, 0, 0,
      SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | flags);
  }
  return TRUE;
}
*/

}
