// MessagesDialog.h

#ifndef __MESSAGES_DIALOG_H
#define __MESSAGES_DIALOG_H

#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/ListView.h"

#include "MessagesDialogRes.h"

class CMessagesDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CListView _messageList;
  
  void AddMessageDirect(LPCWSTR message);
  void AddMessage(LPCWSTR message);
  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  bool OnGetMinMaxInfo(PMINMAXINFO pMMI);
protected:
  SIZE m_sizeMinWindow;
public:
  const UStringVector *Messages;
  CMessagesDialog() { m_sizeMinWindow.cx = 0; m_sizeMinWindow.cy = 0; }
  
  INT_PTR Create(HWND parent = 0) { return CModalDialog::Create(IDD_MESSAGES, parent); }
  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif
