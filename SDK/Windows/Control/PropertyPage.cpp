// Windows/Control/PropertyPage.cpp

#include "StdAfx.h"

#include "Windows/Control/PropertyPage.h"

namespace NWindows {
namespace NControl {

BOOL APIENTRY ProperyPageProcedure(HWND aDialogHWND, UINT aMessage, 
    UINT wParam, LONG lParam)
{
  CDialog aDialogTmp(aDialogHWND);
  if (aMessage == WM_INITDIALOG)
    aDialogTmp.SetUserDataLongPtr(((PROPSHEETPAGE *)lParam)->lParam);
  CDialog *aDialog = (CDialog *)(aDialogTmp.GetUserDataLongPtr());
  if (aMessage == WM_INITDIALOG)
    aDialog->Attach(aDialogHWND);
  switch (aMessage)
  {
    case WM_INITDIALOG:
      return aDialog->OnInit();
    case WM_COMMAND:
      return aDialog->OnCommand(wParam, lParam);
    case WM_NOTIFY:
      return aDialog->OnNotify(wParam, (LPNMHDR) lParam);
  }
  return false;
}

bool CPropertyPage::OnNotify(UINT aControlID, LPNMHDR lParam) 
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
