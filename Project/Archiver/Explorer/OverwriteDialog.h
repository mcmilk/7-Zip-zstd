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
	  FILETIME Time;
    CSysString Name;
  };
}

class COverwriteDialog: public NWindows::NControl::CModalDialog
{
  void SetFileInfoControl(int aTextID, int anIconID, const NOverwriteDialog::CFileInfo &aFileInfo);
  virtual bool OnInit();
  bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
public:
  INT_PTR Create(HWND aWndParent = 0)
    { return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_OVERWRITE), aWndParent); }

  NOverwriteDialog::CFileInfo m_OldFileInfo;
  NOverwriteDialog::CFileInfo m_NewFileInfo;
};

#endif
