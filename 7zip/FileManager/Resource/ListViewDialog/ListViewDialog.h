// ListViewDialog.h

#ifndef __LISTVIEWDIALOG_H
#define __LISTVIEWDIALOG_H

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"
#include "resource.h"

class CListViewDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CListView _listView;
  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnNotify(UINT controlID, LPNMHDR header);

public:
  UString Title;
  bool DeleteIsAllowed;
  CSysStringVector Strings;
  bool StringsWereChanged;
  int FocusedItemIndex;

  INT_PTR Create(HWND wndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_LISTVIEW), wndParent); }

  CListViewDialog(): DeleteIsAllowed(false) {}

};

#endif
