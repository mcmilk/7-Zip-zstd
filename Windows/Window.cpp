// Windows/Window.cpp

#include "StdAfx.h"

#include "Windows/Window.h"

namespace NWindows {

bool CWindow::GetText(CSysString &string)
{
  string.Empty();
  int length = GetTextLength();
  if (length == 0)
    return (::GetLastError() != ERROR_SUCCESS);
  length = GetText(string.GetBuffer(length), length + 1);
  string.ReleaseBuffer();
  if (length == 0)
    return (::GetLastError() != ERROR_SUCCESS);
  return true;
}
  
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
