// ColumnsDialog.h

#pragma once

#ifndef __COLUMNSDIALOG_H
#define __COLUMNSDIALOG_H

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"

namespace NColumnsDialog
{
  struct CColumnInfo
  {
    PROPID PropID;
    bool IsVisible;
    UINT32 Order;
    UINT32 Width;
    CSysString Caption;
  };
}

class CColumnsDialog: public NWindows::NControl::CModalDialog
{
	NWindows::NControl::CListView	m_ListView;
	NWindows::NControl::CDialogChildControl	m_ButtonMoveDown;
	NWindows::NControl::CDialogChildControl	m_ButtonMoveUp;
	NWindows::NControl::CDialogChildControl	m_ButtonShow;
	NWindows::NControl::CDialogChildControl	m_ButtonHide;

  NWindows::NControl::CDialogChildControl m_Width;


  bool m_RefreshButtonsEnabled;
  class CRefreshButtons
  {
    bool &m_Var;
  public:
    CRefreshButtons(bool &aVar): m_Var(aVar) { m_Var = false; };
    ~CRefreshButtons() { Enable(); }
    void Enable() { m_Var = true; }
  };

  void AddRows();
  void RefreshButtons();
  void SwapItems(int anIndexOld, int anIndexNew);
  void SetWidthOfItem(int anIndex);
  void SetItem(int anIndex);
public:
  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_COLUMNS), aWndParent); }

  std::vector<NColumnsDialog::CColumnInfo> m_ColumnInfoVector;

  /*
    // Dialog Data
	//{{AFX_DATA(CColumnsDialog)
	CString	m_Width;
	//}}AFX_DATA
  */


protected:

	virtual bool OnInit();
	virtual void OnOK();
	virtual void OnHelp();
	
  bool OnNotify(UINT aControlID, LPNMHDR lParam);
  void OnItemchangedColumnsListview(const NMLISTVIEW *pNMListView);
  
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
  void OnButtonShow(bool aState);
  void OnButtonMoveUp();
	void OnButtonMoveDown();
	void OnButtonSetWidth();
};

#endif
