// MessagesDialog.h

#ifndef __MESSAGESDIALOG_H
#define __MESSAGESDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"

class CMessagesDialog: public NWindows::NControl::CModalDialog
{
	NWindows::NControl::CListView	_messageList;
	void AddMessageDirect(LPCTSTR message);
	void AddMessage(LPCTSTR message);
  virtual bool OnInit();
public:
  const CSysStringVector *Messages;
  INT_PTR Create(HWND parentWindow = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_MESSAGES), parentWindow); }
};

#endif
