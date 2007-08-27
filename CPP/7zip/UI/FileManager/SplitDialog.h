// SplitDialog.h

#ifndef __SPLITDIALOG_H
#define __SPLITDIALOG_H

#include "Common/Types.h"
#include "Windows/Control/Dialog.h"
#include "Windows/Control/ComboBox.h"
#include "SplitDialogRes.h"

class CSplitDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox _pathCombo;
  NWindows::NControl::CComboBox _volumeCombo;
  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  void OnButtonSetPath();
public:
  UString FilePath;
  UString Path;
  CRecordVector<UInt64> VolumeSizes;
  INT_PTR Create(HWND parentWindow = 0)
    { return CModalDialog::Create(IDD_DIALOG_SPLIT, parentWindow); }
};

#endif
