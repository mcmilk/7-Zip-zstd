// Windows/Control/Dialog.cpp

#include "StdAfx.h"

#include "Windows/Control/Dialog.h"

extern HINSTANCE g_hInstance;

namespace NWindows {
namespace NControl {

BOOL APIENTRY DialogProcedure(HWND dialogHWND, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
  CWindow aDialogTmp(dialogHWND);
  if (message == WM_INITDIALOG)
    aDialogTmp.SetUserDataLongPtr(lParam);
  CDialog *aDialog = (CDialog *)(aDialogTmp.GetUserDataLongPtr());
  if (aDialog == NULL)
    return FALSE;
  if (message == WM_INITDIALOG)
    aDialog->Attach(dialogHWND);

  return BoolToBOOL(aDialog->OnMessage(message, wParam, lParam));
}

bool CDialog::OnMessage(UINT message, UINT wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      return OnInit();
    case WM_COMMAND:
      return OnCommand(wParam, lParam);
    case WM_NOTIFY:
      return OnNotify(wParam, (LPNMHDR) lParam);
    case WM_HELP:
      {
        OnHelp((LPHELPINFO)lParam);
        return true;
      }
    case WM_TIMER:
      {
        return OnTimer(wParam, lParam);
      }
    default:
      return false;
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

bool CDialog::OnButtonClicked(int buttonID, HWND buttonHWND) 
{ 
  switch(buttonID)
  {
    case IDOK:
      OnOK();
      break;
    case IDCANCEL:
      OnCancel();
      break;
    case IDHELP:
      OnHelp();
      break;
    default:
      return false;
  }
  return true;
}

bool CModelessDialog::Create(LPCTSTR templateName, HWND parentWindow)
{ 
  HWND aHWND = CreateDialogParam(g_hInstance, 
      templateName, parentWindow, DialogProcedure, LPARAM(this));
  if (aHWND == 0)
    return false;
  Attach(aHWND);
  return true;
}

INT_PTR CModalDialog::Create(LPCTSTR templateName, HWND parentWindow)
{ 
  return DialogBoxParam(g_hInstance, 
      templateName, parentWindow, DialogProcedure, LPARAM(this));
}

/*
INT_PTR CModalDialog::Create(LPCWSTR templateName, HWND parentWindow)
{ 
  return DialogBoxParamW(g_hInstance, 
      templateName, parentWindow, DialogProcedure, LPARAM(this));
}
*/

}}
