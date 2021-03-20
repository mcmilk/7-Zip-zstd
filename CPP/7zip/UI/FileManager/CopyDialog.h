// CopyDialog.h

#ifndef __COPY_DIALOG_H
#define __COPY_DIALOG_H

#include "../../../Windows/Control/ComboBox.h"
#include "../../../Windows/Control/Dialog.h"

#include "CopyDialogRes.h"

const int kCopyDialog_NumInfoLines = 14;

class CCopyDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox _path;
  NWindows::NControl::CStatic _freeSpace;
  virtual void OnOK();
  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  void OnButtonSetPath();
  void OnButtonOpenPath();
  void OnButtonAddFileName();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
  bool OnCommand(int code, int itemID, LPARAM lParam);
  bool OnGetMinMaxInfo(PMINMAXINFO pMMI);
  void ShowPathFreeSpace(UString & strPath);

protected:
  SIZE m_sizeMinWindow;

public:
  CCopyDialog(): m_bOpenOutputFolder(false), m_bDeleteSourceFile(false), m_bClose7Zip (false) { m_sizeMinWindow.cx = 0; m_sizeMinWindow.cy = 0; }

  UString Title;
  UString Static;
  UString Value;
  UString Info;
  UStringVector Strings;

  bool m_bOpenOutputFolder;
  bool m_bDeleteSourceFile;
  bool m_bClose7Zip;

  UString m_currentFolderPrefix;
  UString m_strRealFileName;

  INT_PTR Create(HWND parentWindow = 0) { return CModalDialog::Create(IDD_COPY, parentWindow); }

  bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif
