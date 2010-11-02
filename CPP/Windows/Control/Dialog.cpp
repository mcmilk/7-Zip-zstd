// Windows/Control/Dialog.cpp

#include "StdAfx.h"

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif
#include "Windows/Control/Dialog.h"

extern HINSTANCE g_hInstance;
#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NControl {

static INT_PTR APIENTRY DialogProcedure(HWND dialogHWND, UINT message, WPARAM wParam, LPARAM lParam)
{
  CWindow dialogTmp(dialogHWND);
  if (message == WM_INITDIALOG)
    dialogTmp.SetUserDataLongPtr(lParam);
  CDialog *dialog = (CDialog *)(dialogTmp.GetUserDataLongPtr());
  if (dialog == NULL)
    return FALSE;
  if (message == WM_INITDIALOG)
    dialog->Attach(dialogHWND);

  try { return BoolToBOOL(dialog->OnMessage(message, wParam, lParam)); }
  catch(...) { return true; }
}

bool CDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG: return OnInit();
    case WM_COMMAND: return OnCommand(wParam, lParam);
    case WM_NOTIFY: return OnNotify((UINT)wParam, (LPNMHDR) lParam);
    case WM_TIMER: return OnTimer(wParam, lParam);
    case WM_SIZE: return OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
    case WM_HELP: OnHelp(); return true;
    /*
        OnHelp(
          #ifdef UNDER_CE
          (void *)
          #else
          (LPHELPINFO)
          #endif
          lParam);
        return true;
    */
    default: return false;
  }
}

bool CDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
  return OnCommand(HIWORD(wParam), LOWORD(wParam), lParam);
}

bool CDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == BN_CLICKED)
    return OnButtonClicked(itemID, (HWND)lParam);
  return false;
}

bool CDialog::OnButtonClicked(int buttonID, HWND /* buttonHWND */)
{
  switch(buttonID)
  {
    case IDOK: OnOK(); break;
    case IDCANCEL: OnCancel(); break;
    case IDHELP: OnHelp(); break;
    default: return false;
  }
  return true;
}

static bool GetWorkAreaRect(RECT *rect)
{
  // use another function for multi-monitor.
  return BOOLToBool(::SystemParametersInfo(SPI_GETWORKAREA, NULL, rect, NULL));
}

bool IsDialogSizeOK(int xSize, int ySize)
{
  // it returns for system font. Real font uses another values
  LONG v = GetDialogBaseUnits();
  int x = LOWORD(v);
  int y = HIWORD(v);

  RECT rect;
  GetWorkAreaRect(&rect);
  int wx = RECT_SIZE_X(rect);
  int wy = RECT_SIZE_Y(rect);
  return
    xSize / 4 * x <= wx &&
    ySize / 8 * y <= wy;
}

void CDialog::NormalizeSize(bool fullNormalize)
{
  RECT workRect;
  GetWorkAreaRect(&workRect);
  int xSize = RECT_SIZE_X(workRect);
  int ySize = RECT_SIZE_Y(workRect);
  RECT rect;
  GetWindowRect(&rect);
  int xSize2 = RECT_SIZE_X(rect);
  int ySize2 = RECT_SIZE_Y(rect);
  bool needMove = (xSize2 > xSize || ySize2 > ySize);
  if (xSize2 > xSize || (needMove && fullNormalize))
  {
    rect.left = workRect.left;
    rect.right = workRect.right;
    xSize2 = xSize;
  }
  if (ySize2 > ySize || (needMove && fullNormalize))
  {
    rect.top = workRect.top;
    rect.bottom = workRect.bottom;
    ySize2 = ySize;
  }
  if (needMove)
  {
    if (fullNormalize)
      Show(SW_SHOWMAXIMIZED);
    else
      Move(rect.left, rect.top, xSize2, ySize2, true);
  }
}

void CDialog::NormalizePosition()
{
  RECT workRect, rect;
  GetWorkAreaRect(&workRect);
  GetWindowRect(&rect);
  if (rect.bottom > workRect.bottom && rect.top > workRect.top)
    Move(rect.left, workRect.top, RECT_SIZE_X(rect), RECT_SIZE_Y(rect), true);
}

bool CModelessDialog::Create(LPCTSTR templateName, HWND parentWindow)
{
  HWND aHWND = CreateDialogParam(g_hInstance, templateName, parentWindow, DialogProcedure, (LPARAM)this);
  if (aHWND == 0)
    return false;
  Attach(aHWND);
  return true;
}

INT_PTR CModalDialog::Create(LPCTSTR templateName, HWND parentWindow)
{
  return DialogBoxParam(g_hInstance, templateName, parentWindow, DialogProcedure, (LPARAM)this);
}

#ifndef _UNICODE

bool CModelessDialog::Create(LPCWSTR templateName, HWND parentWindow)
{
  HWND aHWND;
  if (g_IsNT)
    aHWND = CreateDialogParamW(g_hInstance, templateName, parentWindow, DialogProcedure, (LPARAM)this);
  else
  {
    AString name;
    LPCSTR templateNameA;
    if (IS_INTRESOURCE(templateName))
      templateNameA = (LPCSTR)templateName;
    else
    {
      name = GetSystemString(templateName);
      templateNameA = name;
    }
    aHWND = CreateDialogParamA(g_hInstance, templateNameA, parentWindow, DialogProcedure, (LPARAM)this);
  }
  if (aHWND == 0)
    return false;
  Attach(aHWND);
  return true;
}

INT_PTR CModalDialog::Create(LPCWSTR templateName, HWND parentWindow)
{
  if (g_IsNT)
    return DialogBoxParamW(g_hInstance, templateName, parentWindow, DialogProcedure, (LPARAM)this);
  AString name;
  LPCSTR templateNameA;
  if (IS_INTRESOURCE(templateName))
    templateNameA = (LPCSTR)templateName;
  else
  {
    name = GetSystemString(templateName);
    templateNameA = name;
  }
  return DialogBoxParamA(g_hInstance, templateNameA, parentWindow, DialogProcedure, (LPARAM)this);
}
#endif

}}
