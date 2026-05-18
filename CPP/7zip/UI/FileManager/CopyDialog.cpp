// CopyDialog.cpp

#include "StdAfx.h"

#include "../../../Common/StringConvert.h"

#include "../../../Windows/FileFind.h"
#include "../../../Windows/FileName.h"

#include "../../../Windows/Control/Static.h"

#include "../Common/ZipRegistry.h"

#include "BrowseDialog.h"
#include "CopyDialog.h"
#include "FormatUtils.h"

#include "Panel.h"
#include "ViewSettings.h"

#include "LangUtils.h"

extern void FormatPathFreeSpace(UString &strPath, UString &strText);

using namespace NWindows;

static bool IsFileExistentAndNotDir(const wchar_t * lpszFile)
{
  DWORD dwAttr = GetFileAttributesW(lpszFile);
  return (dwAttr != INVALID_FILE_ATTRIBUTES)
      && ((dwAttr & FILE_ATTRIBUTE_ARCHIVE) != 0)
      && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

// Walk up, extract archive basename.
static void FindRealFileName_WalkUp(UString &pathPrefix, UString &realName)
{
  while (!pathPrefix.IsEmpty())
  {
    if (pathPrefix.Back() == L'\\')
      pathPrefix.DeleteBack();

    if (IsFileExistentAndNotDir(pathPrefix))
    {
      const int slash = pathPrefix.ReverseFind(L'\\');
      const int dot = pathPrefix.ReverseFind(L'.');
      const int start = (slash != -1) ? slash + 1 : 0;
      const int end = (dot == -1 || dot <= start) ? pathPrefix.Len() : dot;
      realName = pathPrefix.Mid(start, end - start);
      return;
    }

    const int slash = pathPrefix.ReverseFind(L'\\');
    if (slash == -1)
      return;
    pathPrefix.ReleaseBuf_SetEnd(slash);
  }
}

bool CCopyDialog::OnInit()
{
  #ifdef Z7_LANG
  LangSetDlgItems(*this, NULL, 0);
  #endif
  _path.Attach(GetItem(IDC_COPY));
  SetText(Title);

  _freeSpace.Attach(GetItem(IDC_FREE_SPACE));
  _freeSpace.SetText(L"");

  NControl::CStatic staticContol;
  staticContol.Attach(GetItem(IDT_COPY));
  staticContol.SetText(Static);
  #ifdef UNDER_CE
  // we do it, since WinCE selects Value\something instead of Value !!!!
  _path.AddString(Value);
  #endif
  FOR_VECTOR (i, Strings)
    _path.AddString(Strings[i]);
  _path.SetText(Value);
  ShowPathFreeSpace(Value);

  OpenOutputFolder = NExtract::Read_OpnTrgFold();
  Close7Zip = Read_Close7Zip();

  CheckButton(IDC_CHECK_OPEN_OUTPUT_FOLDER, OpenOutputFolder);
  CheckButton(IDC_CHECK_CLOSE_7ZIP, Close7Zip);

  SetItemText(IDT_COPY_INFO, Info);
  NormalizeSize(true);

  Set_MinTrackSize_FromCurrent(4, 5, 4, 5);

  /////////////////////////////////////////////////////////

  RealFileName.Empty();
  if (NFile::NFind::DoesDirExist_FollowLink(us2fs(CurrentFolderPrefix)))
  {
    EnableItem(IDC_COPY_ADD_FILE_NAME, false);
    EnableItem(IDC_CHECK_DELETE_SOURCE_FILE, false);
  }
  else
  {
    FindRealFileName_WalkUp(CurrentFolderPrefix, RealFileName);
  }

  return CModalDialog::OnInit();
}

bool CCopyDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int mx, my;
  GetMargins(8, mx, my);
  int bx1, bx2, by;
  GetItemSizes(IDCANCEL, bx1, by);
  GetItemSizes(IDOK, bx2, by);
  const int y = ySize - my - by;
  const int x = xSize - mx - bx1;

  {
    RECT r;

    GetClientRectOfItem(IDC_COPY_ADD_FILE_NAME, r);
    int bxAddFileName = r.right - r.left;
    int byAddFileName = r.bottom - r.top;
    MoveItem(IDC_COPY_ADD_FILE_NAME, xSize - mx - bxAddFileName, r.top, bxAddFileName, byAddFileName, false);

    GetClientRectOfItem(IDC_COPY_OPEN_PATH, r);
    int bxOpen = r.right - r.left;
    int byOpen = r.bottom - r.top;
    MoveItem(IDC_COPY_OPEN_PATH, xSize - mx - bxOpen - mx/2 - bxAddFileName, r.top, bxOpen, byOpen, false);

    GetClientRectOfItem(IDB_COPY_SET_PATH, r);
    int bxSet = RECT_SIZE_X(r);
    int bySet = RECT_SIZE_Y(r);
    MoveItem(IDB_COPY_SET_PATH, xSize - mx - bxSet - bxOpen - mx - bxAddFileName, r.top, bxSet, bySet, false);
    ChangeSubWindowSizeX(_path, xSize - mx - mx - bxSet - bxOpen - mx - mx/2 - bxAddFileName);
  }

  {
    RECT r;
    GetClientRectOfItem(IDT_COPY_INFO, r);
    const int yPos = r.top;
    const int xc = xSize - mx * 2;
    MoveItem(IDT_COPY_INFO, mx, yPos, xc, y - 2 - yPos, false);

    GetClientRectOfItem(IDC_AFTER_EXTRACT, r);
    MoveItem(IDC_AFTER_EXTRACT, mx, r.top, xc, r.bottom - r.top, false);
  }

  MoveItem(IDCANCEL, x, y, bx1, by, false);
  MoveItem(IDOK, x - mx - bx2, y, bx2, by, false);

  InvalidateRect(NULL);
  return false;
}

bool CCopyDialog::OnButtonClicked(unsigned buttonID, HWND buttonHWND)
{
  switch (buttonID)
  {
    case IDB_COPY_SET_PATH:
      OnButtonSetPath();
      return true;
    case IDC_COPY_OPEN_PATH:
      OnButtonOpenPath();
      return true;
    case IDC_COPY_ADD_FILE_NAME:
      OnButtonAddFileName();
      return true;
    case IDC_CHECK_OPEN_OUTPUT_FOLDER:
      OpenOutputFolder = IsButtonCheckedBool(IDC_CHECK_OPEN_OUTPUT_FOLDER);
      return true;
    case IDC_CHECK_DELETE_SOURCE_FILE:
      DeleteSourceFile = IsButtonCheckedBool(IDC_CHECK_DELETE_SOURCE_FILE);
      return true;
    case IDC_CHECK_CLOSE_7ZIP:
      Close7Zip = IsButtonCheckedBool(IDC_CHECK_CLOSE_7ZIP);
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CCopyDialog::OnButtonSetPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  const UString title = LangString(IDS_SET_FOLDER);

  UString resultPath;
  if (!MyBrowseForFolder(*this, title, currentPath, resultPath))
    return;
  NFile::NName::NormalizeDirPathPrefix(resultPath);
  _path.SetCurSel(-1);
  _path.SetText(resultPath);
  ShowPathFreeSpace(resultPath);
}

void CCopyDialog::OnOK()
{
  NExtract::Save_OpnTrgFold(OpenOutputFolder);
  Save_Close7Zip(Close7Zip);

  _path.GetText(Value);
  CModalDialog::OnOK();
}

void CCopyDialog::OnButtonOpenPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  if (NFile::NFind::DoesDirExist_FollowLink(us2fs(currentPath)))
  {
    StartApplicationDontWait(currentPath, currentPath, *this);
  }
  else
  {
    const UString msg = MyFormatNew(
        L"Folder \"{0}\" is not available yet.\n\n"
        L"Note: the program will create the folder automatically when extracting.",
        currentPath);
    MessageBoxW(*this, msg, L"7-Zip", MB_ICONEXCLAMATION);
  }
}

void CCopyDialog::OnButtonAddFileName()
{
  UString currentPath;
  _path.GetText(currentPath);

  currentPath.Trim();
  if (currentPath.Back() == '\\')
  {
    currentPath.DeleteBack();
  }

  UString strLastDir;
  int n = currentPath.ReverseFind(L'\\');
  if (n != -1)
  {
    strLastDir = currentPath.Ptr(n+1);
  }
  else
  {
    strLastDir = currentPath;
  }
  if (strLastDir != RealFileName)
  {
    currentPath += L'\\';
    currentPath += RealFileName;

    _path.SetText(currentPath);
  }
  _path.SetFocus();
}

void CCopyDialog::ShowPathFreeSpace(UString & strPath)
{
  UString strText;
  FormatPathFreeSpace(strPath, strText);
  _freeSpace.SetText(strText);
}

bool CCopyDialog::OnCommand(unsigned code, unsigned itemID, LPARAM lParam)
{
  if (itemID == IDC_COPY)
  {
    if (code == CBN_EDITCHANGE)
    {
      UString strPath;
      _path.GetText(strPath);

      ShowPathFreeSpace(strPath);
      return true;
    }
    else if (code == CBN_SELCHANGE)
    {
      int nSel = _path.GetCurSel();
      if (nSel != CB_ERR)
      {
        UString strPath;
        _path.GetLBText(nSel, strPath);
        ShowPathFreeSpace(strPath);
      }
      return true;
    }
  }
  return CModalDialog::OnCommand(code, itemID, lParam);
}
