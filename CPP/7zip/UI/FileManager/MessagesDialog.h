// MessagesDialog.h

#ifndef __MESSAGESDIALOG_H
#define __MESSAGESDIALOG_H

#include "MessagesDialogRes.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"

class CMessagesDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CListView _messageList;
  void AddMessageDirect(LPCWSTR message);
  void AddMessage(LPCWSTR message);
  virtual bool OnInit();
public:
  const UStringVector *Messages;
  INT_PTR Create(HWND parent = 0) { return CModalDialog::Create(IDD_DIALOG_MESSAGES, parent); }
};

#endif
