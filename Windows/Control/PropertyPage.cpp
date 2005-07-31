// Windows/Control/PropertyPage.cpp

#include "StdAfx.h"

#include "Windows/Control/PropertyPage.h"

namespace NWindows {
namespace NControl {

INT_PTR APIENTRY ProperyPageProcedure(HWND dialogHWND, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
  CDialog tempDialog(dialogHWND);
  if (message == WM_INITDIALOG)
    tempDialog.SetUserDataLongPtr(((PROPSHEETPAGE *)lParam)->lParam);
  CDialog *dialog = (CDialog *)(tempDialog.GetUserDataLongPtr());
  if (message == WM_INITDIALOG)
    dialog->Attach(dialogHWND);
  switch (message)
  {
    case WM_INITDIALOG:
      return dialog->OnInit();
    case WM_COMMAND:
      return dialog->OnCommand(wParam, lParam);
    case WM_NOTIFY:
      return dialog->OnNotify(wParam, (LPNMHDR) lParam);
  }
  if (dialog == NULL)
    return false;
  return dialog->OnMessage(message, wParam, lParam);
}

bool CPropertyPage::OnNotify(UINT controlID, LPNMHDR lParam) 
{
  switch(lParam->code)
  {
    case PSN_APPLY:
      SetMsgResult(OnApply(LPPSHNOTIFY(lParam)));
      break;
    case PSN_KILLACTIVE:
      SetMsgResult(BoolToBOOL(OnKillActive(LPPSHNOTIFY(lParam))));
      break;
    case PSN_SETACTIVE:
      SetMsgResult(OnSetActive(LPPSHNOTIFY(lParam)));
      break;
    case PSN_RESET:
      OnReset(LPPSHNOTIFY(lParam));
      break;
    case PSN_HELP:
      OnNotifyHelp(LPPSHNOTIFY(lParam));
      break;
    default:
      return false;
  }
  return true;
}


}}
