// Windows/Control/Dialog.h

#pragma once

#ifndef __WINDOWS_CONTROL_DIALOG_H
#define __WINDOWS_CONTROL_DIALOG_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

BOOL APIENTRY DialogProcedure(HWND aDialogHWND, UINT aMessage, UINT wParam, LPARAM lParam);

class CDialog: public CWindow
{
public:
  CDialog(HWND aWindowNew = NULL): CWindow(aWindowNew){};
  virtual ~CDialog() {};

  bool End(INT_PTR aResult)
    { return BOOLToBool(EndDialog(m_Window, aResult)); }

  HWND GetItem(int anItemID) const
    { return GetDlgItem(m_Window, anItemID); }

  bool SetItemText(int anItemID, LPCTSTR aString)
    { return BOOLToBool(SetDlgItemText(m_Window, anItemID, aString)); }
  UINT GetItemText(int anItemID, LPTSTR aString, int aMaxCount)
    { return GetDlgItemText(m_Window, anItemID, aString, aMaxCount); }
  bool SetItemInt(int anItemID, UINT aValue, bool aSigned)
    { return BOOLToBool(SetDlgItemInt(m_Window, anItemID, aValue, BoolToBOOL(aSigned))); }
  bool GetItemInt(int anItemID, bool aSigned, UINT &aValue)
    { 
      BOOL aResult;
      aValue = GetDlgItemInt(m_Window, aValue, &aResult, BoolToBOOL(aSigned));
      return BOOLToBool(aResult);
    }

  HWND GetNextGroupItem(HWND aControl, bool aPrevious)
    { return GetNextDlgGroupItem(m_Window, aControl, BoolToBOOL(aPrevious)); }
  HWND GetNextTabItem(HWND aControl, bool aPrevious)
    { return GetNextDlgTabItem(m_Window, aControl, BoolToBOOL(aPrevious)); }

  bool MapRect(LPRECT aRect)
    { return BOOLToBool(MapDialogRect(m_Window, aRect)); }

  bool IsMessage(LPMSG aMessage)
    { return BOOLToBool(IsDialogMessage(m_Window, aMessage)); }

  LRESULT SendItemMessage(int anItemID, UINT aMessage, WPARAM wParam, LPARAM lParam)
    { return SendDlgItemMessage(m_Window, anItemID, aMessage, wParam, lParam); }

  bool CheckButton(int aButtonID, UINT aCheckState)
    { return BOOLToBool(CheckDlgButton(m_Window, aButtonID, aCheckState)); }
  bool CheckButton(int aButtonID, bool aCheckState)
    { return CheckButton(aButtonID, UINT(aCheckState ? BST_CHECKED : BST_UNCHECKED)); }

  UINT IsButtonChecked(int aButtonID) const
    { return IsDlgButtonChecked(m_Window, aButtonID); }
  bool IsButtonCheckedBool(int aButtonID) const
    { return (IsButtonChecked(aButtonID) == BST_CHECKED); }

  bool CheckRadioButton(int aFirstButtonID, int nLastButtonID, int aCheckButtonID)
    { return BOOLToBool(::CheckRadioButton(m_Window, aFirstButtonID, nLastButtonID, aCheckButtonID)); }

  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
  virtual bool OnInit() { return true; }
  virtual bool OnCommand(WPARAM wParam, LPARAM lParam);
  virtual bool OnCommand(int aCode, int anItemID, LPARAM lParam);
  virtual void OnHelp(LPHELPINFO aHelpInfo) { OnHelp(); };
  virtual void OnHelp() {};
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
  virtual void OnOK() {};
  virtual void OnCancel() {};
  virtual bool OnNotify(UINT aControlID, LPNMHDR lParam) { return false; }

  LONG_PTR SetMsgResult(LONG_PTR aNewLongPtr )
    { return SetLongPtr(DWLP_MSGRESULT, aNewLongPtr); }
  LONG_PTR GetMsgResult() const
    { return GetLongPtr(DWLP_MSGRESULT); }
};

class CModelessDialog: public CDialog
{
public:
  bool Create(LPCTSTR aTemplateName, HWND hWndParent);
  virtual void OnOK() { Destroy(); }
  virtual void OnCancel() { Destroy(); }
};

class CModalDialog: public CDialog
{
public:
  INT_PTR Create(LPCTSTR aTemplateName, HWND hWndParent);
  virtual void OnOK() { End(IDOK); }
  virtual void OnCancel() { End(IDCANCEL); }
};

class CDialogChildControl: public NWindows::CWindow
{
public:
  int m_ID;
  void Init(const NWindows::NControl::CDialog &aParentDialog, int anID)
  {
    m_ID = anID;
    Attach(aParentDialog.GetItem(anID));
  }
};

}}

#endif