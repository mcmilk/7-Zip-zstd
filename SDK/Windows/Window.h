// Windows/Window.h

#pragma once

#ifndef __WINDOWS_WINDOW_H
#define __WINDOWS_WINDOW_H

#include "Windows/Defs.h"
#include "Common/String.h"

namespace NWindows {

class CWindow
{
private:
   // bool ModifyStyleBase(int aStyleOffset, DWORD aRemove, DWORD anAdd, UINT aFlags);
protected:
  HWND m_Window;
public:
  CWindow(HWND aWindowNew = NULL): m_Window(aWindowNew){};
  CWindow& operator=(HWND aWindowNew)
  {
    m_Window = aWindowNew;
    return *this;
  }
  operator HWND() const { return m_Window; }
  void Attach(HWND aWindowNew)
    { m_Window = aWindowNew; }
  HWND Detach()
  {
    HWND aWindow = m_Window;
    m_Window = NULL;
    return aWindow;
  }

  HWND GetParent() const 
    { return ::GetParent(m_Window); }
  bool GetWindowRect(LPRECT aRect) const
    { return BOOLToBool(::GetWindowRect(m_Window,aRect )); }

  bool ClientToScreen(LPPOINT aPoint) const
    { return BOOLToBool(::ClientToScreen(m_Window, aPoint)); }

  bool ScreenToClient(LPPOINT aPoint) const
    { return BOOLToBool(::ScreenToClient(m_Window, aPoint)); }

  bool CreateEx(DWORD anExStyle, LPCTSTR aClassName,
      LPCTSTR aWindowName, DWORD aStyle,
      int x, int y, int aWidth, int aHeight,
      HWND aParentWindow, HMENU anIDorHMenu, 
      HINSTANCE anInstance, LPVOID aCreateParam)
  {
    m_Window = ::CreateWindowEx(anExStyle, aClassName, aWindowName,
      aStyle, x, y, aWidth, aHeight, aParentWindow, 
      anIDorHMenu, anInstance, aCreateParam);
    return (m_Window != NULL);
  }

  bool Destroy()
  {
    if (m_Window == NULL)
      return true;
    bool aResult = BOOLToBool(::DestroyWindow(m_Window));
    if(aResult)
      m_Window = NULL;
    return aResult;
  }
  bool IsWindow()
    {  return BOOLToBool(::IsWindow(m_Window)); }
  bool MoveWindow(int x, int y, int aWidth, int aHeight, bool aRepaint = true)
    { return BOOLToBool(::MoveWindow(m_Window, x, y, aWidth, aHeight, BoolToBOOL(aRepaint))); }
  bool GetClientRect(LPRECT aRect)
    { return BOOLToBool(::GetClientRect(m_Window, aRect)); }
  bool ShowWindow(int aCmdShow)
    { return BOOLToBool(::ShowWindow(m_Window, aCmdShow)); }
  bool UpdateWindow()
    { return BOOLToBool(::UpdateWindow(m_Window)); }
  bool InvalidateRect(LPCRECT aRect, bool aBackgroundErase = true)
    { return BOOLToBool(::InvalidateRect(m_Window, aRect, BoolToBOOL(aBackgroundErase))); }
  void SetRedraw(bool aRedraw = true)
    { SendMessage(WM_SETREDRAW, BoolToBOOL(aRedraw), 0); }

  #ifndef _WIN32_WCE
  LONG SetStyle(LONG_PTR aStyle)
    { return SetLongPtr(GWL_STYLE, aStyle); }
  DWORD GetStyle( ) const
    { return GetLongPtr(GWL_STYLE); }
  #else
  LONG SetStyle(LONG_PTR aStyle)
    { return SetLong(GWL_STYLE, aStyle); }
  DWORD GetStyle( ) const
    { return GetLong(GWL_STYLE); }
  #endif

  LONG_PTR SetLong(int anIndex, LONG_PTR aNewLongPtr )
    { return ::SetWindowLong(m_Window, anIndex, aNewLongPtr); }
  LONG_PTR GetLong(int anIndex) const
    { return ::GetWindowLong(m_Window, anIndex ); }
  LONG_PTR SetUserDataLong(LONG_PTR aNewLongPtr )
    { return SetLong(GWL_USERDATA, aNewLongPtr); }
  LONG_PTR GetUserDataLong() const
    { return GetLong(GWL_USERDATA); }

  #ifndef _WIN32_WCE
  LONG_PTR SetLongPtr(int anIndex, LONG_PTR aNewLongPtr )
    { return ::SetWindowLongPtr(m_Window, anIndex, aNewLongPtr); }
  LONG_PTR GetLongPtr(int anIndex) const
    { return ::GetWindowLongPtr(m_Window, anIndex ); }
  LONG_PTR SetUserDataLongPtr(LONG_PTR aNewLongPtr )
    { return SetLongPtr(GWLP_USERDATA, aNewLongPtr); }
  LONG_PTR GetUserDataLongPtr() const
    { return GetLongPtr(GWLP_USERDATA); }
  #endif
  
  /*
  bool ModifyStyle(HWND hWnd, DWORD aRemove, DWORD anAdd, UINT aFlags = 0)
    {  return ModifyStyleBase(GWL_STYLE, aRemove, anAdd, aFlags); }
  bool ModifyStyleEx(HWND hWnd, DWORD aRemove, DWORD anAdd, UINT aFlags = 0)
    { return ModifyStyleBase(GWL_EXSTYLE, aRemove, anAdd, aFlags); }
  */
 
  HWND SetFocus()
    { return ::SetFocus(m_Window); }

  LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {  return ::SendMessage(m_Window, message, wParam, lParam) ;}
  bool PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0)
    {  return BOOLToBool(::PostMessage(m_Window, message, wParam, lParam)) ;}

  bool SetText(LPCTSTR aString)
    { return BOOLToBool(::SetWindowText(m_Window, aString)); }
  int GetTextLength() const 
    { return GetWindowTextLength(m_Window); }
  UINT GetText(LPTSTR aString, int aMaxCount) const
    { return GetWindowText(m_Window, aString, aMaxCount); }
  bool GetText(CSysString &aString);

  bool Enable(bool anEnable)
    { return BOOLToBool(::EnableWindow(m_Window, BoolToBOOL(anEnable))); }
  
  bool IsEnabled()
    { return BOOLToBool(::IsWindowEnabled(m_Window)); }
  
  #ifndef _WIN32_WCE
  HMENU GetSystemMenu(bool aRevert)
    { return ::GetSystemMenu(m_Window, BoolToBOOL(aRevert)); }
  #endif

};

}

#endif

