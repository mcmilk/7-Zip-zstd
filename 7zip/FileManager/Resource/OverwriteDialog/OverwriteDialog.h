// OverwriteDialog.h

#pragma once

#ifndef __OVERWRITEDIALOG_H
#define __OVERWRITEDIALOG_H

#include "resource.h"
#include "Windows/Control/Dialog.h"

namespace NOverwriteDialog
{
  struct CFileInfo
  {
    bool SizeIsDefined;
    UINT64 Size;
    bool TimeIsDefined;
	  FILETIME Time;
    CSysString Name;
  };
}

class COverwriteDialog: public NWindows::NControl::CModalDialog
{
  void SetFileInfoControl(int textID, int iconID, 
      const NOverwriteDialog::CFileInfo &fileInfo);
  virtual bool OnInit();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
  INT_PTR Create(HWND parent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_OVERWRITE), parent); }

  NOverwriteDialog::CFileInfo _oldFileInfo;
  NOverwriteDialog::CFileInfo _newFileInfo;
};

#endif
