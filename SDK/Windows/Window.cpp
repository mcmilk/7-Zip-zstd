// Windows::Window.cpp

#include "StdAfx.h"

#include "Windows/Window.h"

namespace NWindows {

bool CWindow::GetText(CSysString &aString)
{
  aString.Empty();
  int aLength = GetTextLength();
  if (aLength == 0)
    return (::GetLastError() != ERROR_SUCCESS);
  aLength = GetText(aString.GetBuffer(aLength), aLength + 1);
  aString.ReleaseBuffer();
  if (aLength == 0)
    return (::GetLastError() != ERROR_SUCCESS);
  return true;
}
  
/*
bool CWindow::ModifyStyleBase(int aStyleOffset,
  DWORD aRemove, DWORD anAdd, UINT aFlags)
{
  DWORD aStyle = GetWindowLong(aStyleOffset);
  DWORD aNewStyle = (aStyle & ~aRemove) | anAdd;
  if (aStyle == aNewStyle)
    return false; // it is not good 

  SetWindowLong(aStyleOffset, aNewStyle);
  if (aFlags != 0)
  {
    ::SetWindowPos(m_Window, NULL, 0, 0, 0, 0,
      SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | aFlags);
  }
  return TRUE;
}
*/

}
