// OverwriteDialog.h

#ifndef __OVERWRITEDIALOG_H
#define __OVERWRITEDIALOG_H

#include "OverwriteDialogRes.h"
#include "Windows/Control/Dialog.h"

namespace NOverwriteDialog
{
  struct CFileInfo
  {
    bool SizeIsDefined;
    UINT64 Size;
    bool TimeIsDefined;
    FILETIME Time;
    UString Name;
  };
}

class COverwriteDialog: public NWindows::NControl::CModalDialog
{
  void SetFileInfoControl(int textID, int iconID, 
      const NOverwriteDialog::CFileInfo &fileInfo);
  virtual bool OnInit();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
public:
  INT_PTR Create(HWND parent = 0) { return CModalDialog::Create(IDD_DIALOG_OVERWRITE, parent); }

  NOverwriteDialog::CFileInfo OldFileInfo;
  NOverwriteDialog::CFileInfo NewFileInfo;
};

#endif
