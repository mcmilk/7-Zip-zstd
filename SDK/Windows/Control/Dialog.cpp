// Windows/Control/Dialog.cpp

#include "StdAfx.h"

#include "Windows/Control/Dialog.h"

extern HINSTANCE g_hInstance;

namespace NWindows {
namespace NControl {

BOOL APIENTRY DialogProcedure(HWND aDialogHWND, UINT aMessage, 
    WPARAM wParam, LPARAM lParam)
{
  CWindow aDialogTmp(aDialogHWND);
  if (aMessage == WM_INITDIALOG)
    aDialogTmp.SetUserDataLongPtr(lParam);
  CDialog *aDialog = (CDialog *)(aDialogTmp.GetUserDataLongPtr());
  if (aDialog == NULL)
    return FALSE;
  if (aMessage == WM_INITDIALOG)
    aDialog->Attach(aDialogHWND);

  return BoolToBOOL(aDialog->OnMessage(aMessage, wParam, lParam));
}

bool CDialog::OnMessage(UINT aMessage, UINT wParam, LPARAM lParam)
{
  switch (aMessage)
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
    default:
      return false;
  }
}

bool CDialog::OnCommand(WPARAM wParam, LPARAM lParam) 
{ 
  return OnCommand(HIWORD(wParam), LOWORD(wParam), lParam);
}

bool CDialog::OnCommand(int aCode, int anItemID, LPARAM lParam)
{
  if (aCode == BN_CLICKED)
    return OnButtonClicked(anItemID, (HWND)lParam);
  return false; 
}

bool CDialog::OnButtonClicked(int aButtonID, HWND aButtonHWND) 
{ 
  switch(aButtonID)
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

bool CModelessDialog::Create(LPCTSTR aTemplateName, HWND hWndParent)
{ 
  HWND aHWND = CreateDialogParam(g_hInstance, 
      aTemplateName, hWndParent, DialogProcedure, LPARAM(this));
  if (aHWND == 0)
    return false;
  Attach(aHWND);
  return true;
}

INT_PTR CModalDialog::Create(LPCTSTR aTemplateName, HWND hWndParent)
{ 
  return DialogBoxParam(g_hInstance, 
      aTemplateName, hWndParent, DialogProcedure, LPARAM(this));
}

}}
