// ListViewDialog.h

#pragma once

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
public:
  CSysString Title;
  CSysStringVector Strings;
  int ItemIndex;

  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_LISTVIEW), aWndParent); }
};

#endif
