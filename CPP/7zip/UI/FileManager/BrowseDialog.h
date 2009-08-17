// BrowseDialog.h

#ifndef __BROWSE_DIALOG_H
#define __BROWSE_DIALOG_H

#ifdef UNDER_CE

#include "Windows/FileFind.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"

#include "BrowseDialogRes.h"
#include "SysIconUtils.h"

class CBrowseDialog: public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CListView _list;
  CObjectVector<NWindows::NFile::NFind::CFileInfoW> _files;
  CExtToIconMap _extToIconMap;
  int _sortIndex;
  bool _ascending;
  bool _showDots;

  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  virtual bool OnNotify(UINT controlID, LPNMHDR header);
  virtual void OnOK();

  virtual bool OnButtonClicked(int buttonID, HWND buttonHWND);
  virtual bool OnKeyDown(LPNMLVKEYDOWN keyDownInfo, LRESULT &result);

  void FinishOnOK();
  HRESULT Reload(const UString &pathPrefix, const UString &selectedName);
  HRESULT Reload();
  void OpenParentFolder();

  void OnItemEnter();

  int GetRealItemIndex(int indexInListView) const
  {
    LPARAM param;
    if (!_list.GetItemParam(indexInListView, param))
      return (int)-1;
    return (int)param;
  }

  void ShowError(LPCWSTR s);
  void ShowSelectError();
public:
  UString Title;
  UString Path;
  bool FolderMode;

  CBrowseDialog(): FolderMode(true), _showDots(false) {}

  INT_PTR Create(HWND parent = 0) { return CModalDialog::Create(IDD_DIALOG_BROWSE, parent); }
  int CompareItems(LPARAM lParam1, LPARAM lParam2);
};

bool MyBrowseForFolder(HWND owner, LPCWSTR title, LPCWSTR initialFolder, UString &resultPath);
bool MyBrowseForFile(HWND owner, LPCWSTR title, LPCWSTR initialFolder, LPCWSTR s, UString &resultPath);

#else

#include "Windows/CommonDialog.h"
#include "Windows/Shell.h"

#define MyBrowseForFolder(h, title, initialFolder, resultPath) \
  NShell::BrowseForFolder(h, title, initialFolder, resultPath)

#define MyBrowseForFile(h, title, initialFolder, s, resultPath) \
  MyGetOpenFileName(h, title, initialFolder, s, resultPath)

#endif

#endif
