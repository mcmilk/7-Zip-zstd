// CopyDialog.cpp

#include "StdAfx.h"

#include "Windows/FileName.h"

#include "Windows/Control/Static.h"

#include "BrowseDialog.h"
#include "CopyDialog.h"

#ifdef LANG
#include "LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif

bool CCopyDialog::OnInit()
{
  #ifdef LANG
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _path.Attach(GetItem(IDC_COPY_COMBO));
  SetText(Title);

  NControl::CStatic staticContol;
  staticContol.Attach(GetItem(IDC_COPY_STATIC));
  staticContol.SetText(Static);
  #ifdef UNDER_CE
  // we do it, since WinCE selects Value\something instead of Value !!!!
  _path.AddString(Value);
  #endif
  for (int i = 0; i < Strings.Size(); i++)
    _path.AddString(Strings[i]);
  _path.SetText(Value);
  SetItemText(IDC_COPY_INFO, Info);
  NormalizeSize(true);
  return CModalDialog::OnInit();
}

bool CCopyDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int mx, my;
  GetMargins(8, mx, my);
  int bx1, bx2, by;
  GetItemSizes(IDCANCEL, bx1, by);
  GetItemSizes(IDOK, bx2, by);
  int y = ySize - my - by;
  int x = xSize - mx - bx1;

  InvalidateRect(NULL);

  {
    RECT rect;
    GetClientRectOfItem(IDC_COPY_SET_PATH, rect);
    int bx = rect.right - rect.left;
    MoveItem(IDC_COPY_SET_PATH, xSize - mx - bx, rect.top, bx, rect.bottom - rect.top);
    ChangeSubWindowSizeX(_path, xSize - mx - mx - bx - mx);
  }

  {
    RECT rect;
    GetClientRectOfItem(IDC_COPY_INFO, rect);
    NControl::CStatic staticContol;
    staticContol.Attach(GetItem(IDC_COPY_INFO));
    int yPos = rect.top;
    staticContol.Move(mx, yPos, xSize - mx * 2, y - 2 - yPos);
  }

  MoveItem(IDCANCEL, x, y, bx1, by);
  MoveItem(IDOK, x - mx - bx2, y, bx2, by);

  return false;
}

bool CCopyDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_COPY_SET_PATH:
      OnButtonSetPath();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CCopyDialog::OnButtonSetPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  UString title = LangStringSpec(IDS_SET_FOLDER, 0x03020209);

  UString resultPath;
  if (!MyBrowseForFolder(HWND(*this), title, currentPath, resultPath))
    return;
  NFile::NName::NormalizeDirPathPrefix(resultPath);
  _path.SetCurSel(-1);
  _path.SetText(resultPath);
}

void CCopyDialog::OnOK()
{
  _path.GetText(Value);
  CModalDialog::OnOK();
}
