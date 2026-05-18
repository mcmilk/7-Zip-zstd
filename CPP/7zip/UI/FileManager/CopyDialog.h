// CopyDialog.h

#ifndef ZIP7_INC_COPY_DIALOG_H
#define ZIP7_INC_COPY_DIALOG_H

#include "../../../Windows/Control/ComboBox.h"
#include "../../../Windows/Control/Dialog.h"
#include "../../../Windows/Control/Static.h"

#include "CopyDialogRes.h"

const int kCopyDialog_NumInfoLines = 11;

class CCopyDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox _path;
  NWindows::NControl::CStatic _freeSpace;
  virtual void OnOK() Z7_override;
  virtual bool OnInit() Z7_override;
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize) Z7_override;
  virtual bool OnButtonClicked(unsigned buttonID, HWND buttonHWND) Z7_override;
  void OnButtonSetPath();
  void OnButtonOpenPath();
  void OnButtonAddFileName();
  bool OnCommand(unsigned code, unsigned itemID, LPARAM lParam) Z7_override;

  void ShowPathFreeSpace(UString & strPath);

public:
  CCopyDialog() = default;

  UString Title;
  UString Static;
  UString Value;
  UString Info;
  UStringVector Strings;

  bool OpenOutputFolder = false;
  bool DeleteSourceFile = false;
  bool Close7Zip = false;

  UString CurrentFolderPrefix;
  UString RealFileName;

  INT_PTR Create(HWND parentWindow = NULL) { return CModalDialog::Create(IDD_COPY, parentWindow); }
};

#endif
