// MessagesDialog.h

#pragma once

#ifndef __MESSAGESDIALOG_H
#define __MESSAGESDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"

class CMessagesDialog: public NWindows::NControl::CModalDialog
{
	NWindows::NControl::CListView	m_MessageList;
	void AddMessage(LPCTSTR aString);
  virtual bool OnInit();
public:
  const CSysStringVector *m_Messages;
  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_MESSAGES), aWndParent); }
};

#endif
