// ExtractDialog.h

#ifndef ZIP7_INC_EXTRACT_DIALOG_H
#define ZIP7_INC_EXTRACT_DIALOG_H

#include "ExtractDialogRes.h"

#include "../../../Windows/Control/ComboBox.h"
#include "../../../Windows/Control/Edit.h"
#include "../../../Windows/Control/Static.h"

#include "../Common/ExtractMode.h"

#include "../FileManager/DialogSize.h"

#ifndef Z7_NO_REGISTRY
#include "../Common/ZipRegistry.h"
#endif

namespace NExtractionDialog
{
  /*
  namespace NFilesMode
  {
    enum EEnum
    {
      kSelected,
      kAll,
      kSpecified
    };
  }
  */
}

class CExtractDialog: public NWindows::NControl::CModalDialog
{
  #ifdef Z7_NO_REGISTRY
  NWindows::NControl::CDialogChildControl _path;
  #else
  NWindows::NControl::CComboBox _path;
  #endif

  #ifndef Z7_SFX
  NWindows::NControl::CEdit _pathName;
  NWindows::NControl::CEdit _passwordControl;
  NWindows::NControl::CComboBox _pathMode;
  NWindows::NControl::CComboBox _overwriteMode;
  #endif
  NWindows::NControl::CStatic _freeSpace;

  #ifndef Z7_SFX
  // int GetFilesMode() const;
  void UpdatePasswordControl();
  #endif

  void OnButtonSetPath();

  void CheckButton_TwoBools(UINT id, const CBoolPair &b1, const CBoolPair &b2);
  void GetButton_Bools(UINT id, CBoolPair &b1, CBoolPair &b2);
  virtual bool OnInit() Z7_override;
  virtual bool OnButtonClicked(unsigned buttonID, HWND buttonHWND) Z7_override;
  virtual void OnOK() Z7_override;
  void OnButtonOpenPath();
  bool OnCommand(unsigned code, unsigned itemID, LPARAM lParam) Z7_override;

  #ifndef Z7_NO_REGISTRY

  virtual void OnHelp() Z7_override;

  NExtract::CInfo _info;

  #endif

  void ShowPathFreeSpace(UString & strPath);
  bool IsShowPasswordChecked() const { return IsButtonCheckedBool(IDX_PASSWORD_SHOW); }
public:
  // bool _enableSelectedFilesButton;
  // bool _enableFilesButton;
  // NExtractionDialog::NFilesMode::EEnum FilesMode;

  UString DirPath;
  UString ArcPath;

  #ifndef Z7_SFX
  UString Password;
  #endif
  bool PathMode_Force = false;
  bool OverwriteMode_Force = false;
  NExtract::NPathMode::EEnum PathMode;
  NExtract::NOverwriteMode::EEnum OverwriteMode;

  bool DeleteSourceFile = false;

  #ifndef Z7_SFX
  // CBoolPair AltStreams;
  CBoolPair NtSecurity;
  CBoolPair OpnTrgFold;
  #endif

  CBoolPair ElimDup;

  INT_PTR Create(HWND aWndParent = NULL)
  {
    #ifdef Z7_SFX
    BIG_DIALOG_SIZE(240, 64);
    #else
    BIG_DIALOG_SIZE(300, 160);
    #endif
    return CModalDialog::Create(SIZED_DIALOG(IDD_EXTRACT), aWndParent);
  }

  CExtractDialog()
  {
    ElimDup.Val = true;
  }

};

#endif
