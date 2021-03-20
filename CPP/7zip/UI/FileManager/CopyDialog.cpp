// CopyDialog.cpp

#include "StdAfx.h"

#include "../../../Windows/FileName.h"

#include "../../../Windows/Control/Static.h"

#include "BrowseDialog.h"
#include "CopyDialog.h"

#include "Panel.h"
#include "ViewSettings.h"

#ifdef LANG
#include "LangUtils.h"
#endif

using namespace NWindows;

static bool IsFileExistentAndNotDir(const wchar_t * lpszFile)
{
  DWORD	dwAttr;
  dwAttr = GetFileAttributesW(lpszFile);
  return (dwAttr != INVALID_FILE_ATTRIBUTES) 
    && ((dwAttr & FILE_ATTRIBUTE_ARCHIVE) != 0)
    && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

static bool IsDirectory(LPCWSTR lpszPathFile)
{
  DWORD	dwAttr;
  dwAttr = GetFileAttributesW(lpszPathFile);
  return (dwAttr != (DWORD)-1) && ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

bool CCopyDialog::OnInit()
{
  #ifdef LANG
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

  m_bOpenOutputFolder = ReadOptOpenOutputFolder();
  m_bClose7Zip = ReadOptClose7Zip();

  CheckButton(IDC_CHECK_OPEN_OUTPUT_FOLDER, m_bOpenOutputFolder);
  CheckButton(IDC_CHECK_CLOSE_7ZIP, m_bClose7Zip);

  SetItemText(IDT_COPY_INFO, Info);
  NormalizeSize(true);

  RECT rc;
  GetWindowRect(&rc);
  m_sizeMinWindow.cx = (RECT_SIZE_X(rc))*4/5;
  m_sizeMinWindow.cy = (RECT_SIZE_Y(rc))*4/5;

  /////////////////////////////////////////////////////////

  m_strRealFileName.Empty();
  if (IsDirectory(m_currentFolderPrefix))
  {
    EnableItem(IDC_COPY_ADD_FILE_NAME, false);
    EnableItem(IDC_CHECK_DELETE_SOURCE_FILE, false);
  }
  else
  {
    while (!m_currentFolderPrefix.IsEmpty())
    {
      if (m_currentFolderPrefix.Back() == '\\')
      {
        m_currentFolderPrefix.DeleteBack();
      }

      if (IsFileExistentAndNotDir(m_currentFolderPrefix))
      {
        int n = m_currentFolderPrefix.ReverseFind(L'\\');
        int m = m_currentFolderPrefix.ReverseFind(L'.');
        if (n != -1)
        {
          n++;
        }
        else
        {
          n = 0;
        }
        if (m == -1 || m <= n) m = m_currentFolderPrefix.Len();
        m_strRealFileName = m_currentFolderPrefix.Mid(n, m - n);
        break;
      }
      else
      {
        int n = m_currentFolderPrefix.ReverseFind(L'\\');
        if (n != -1)
        {
          m_currentFolderPrefix.ReleaseBuf_SetEnd(n);
        }
        else
        {
          break;
        }
      }
    }
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
  int y = ySize - my - by;
  int x = xSize - mx - bx1;

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
    ChangeSubWindowSizeX(_path, xSize - mx - mx - bxSet - bxOpen - mx - mx/2 - bxAddFileName, false);
  }

  {
    RECT r;
    GetClientRectOfItem(IDT_COPY_INFO, r);
    int yPos = r.top;
    int xc = xSize - mx * 2;
    MoveItem(IDT_COPY_INFO, mx, yPos, xc, y - 2 - yPos, false);

    GetClientRectOfItem(IDC_AFTER_EXTRACT, r);
    MoveItem(IDC_AFTER_EXTRACT, mx, r.top, xc, r.bottom-r.top, false);
  }

  MoveItem(IDCANCEL, x, y, bx1, by, false);
  MoveItem(IDOK, x - mx - bx2, y, bx2, by, false);

  InvalidateRect(NULL);
  return false;
}

bool CCopyDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
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
      m_bOpenOutputFolder = IsButtonCheckedBool(IDC_CHECK_OPEN_OUTPUT_FOLDER);
      return true;
    case IDC_CHECK_DELETE_SOURCE_FILE:
      m_bDeleteSourceFile = IsButtonCheckedBool(IDC_CHECK_DELETE_SOURCE_FILE);
      return true;
    case IDC_CHECK_CLOSE_7ZIP:
      m_bClose7Zip = IsButtonCheckedBool(IDC_CHECK_CLOSE_7ZIP);
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
  SaveOptOpenOutputFolder(m_bOpenOutputFolder);
  SaveOptClose7Zip(m_bClose7Zip);

  _path.GetText(Value);
  CModalDialog::OnOK();
}

void CCopyDialog::OnButtonOpenPath()
{
  UString currentPath;
  _path.GetText(currentPath);

  if (IsDirectory(currentPath))
  {
    StartApplicationDontWait(currentPath, currentPath, (HWND)_window);
  }
  else
  {
    WCHAR szMsg[1024];
    wsprintfW(szMsg, L"Folder \"%s\" is not available yet.\n\n"
      L"Note: the program will create the folder automatically when extracting.", (LPCWSTR)currentPath);
    MessageBoxW((HWND)_window, szMsg, L"7-Zip", MB_ICONEXCLAMATION);
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
    strLastDir = currentPath.Mid(n+1);
  }
  else
  {
    strLastDir = currentPath;
  }
  if (strLastDir != m_strRealFileName)
  {
    currentPath += L'\\';
    currentPath += m_strRealFileName;

    _path.SetText(currentPath);
  }
  _path.SetFocus();
}

bool CCopyDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
  case WM_GETMINMAXINFO:
    {
      return OnGetMinMaxInfo((PMINMAXINFO)lParam);
    }
  }
  return CModalDialog::OnMessage(message, wParam, lParam);
}

bool CCopyDialog::OnGetMinMaxInfo(PMINMAXINFO pMMI)
{
  pMMI->ptMinTrackSize.x = m_sizeMinWindow.cx;
  pMMI->ptMinTrackSize.y = m_sizeMinWindow.cy;
  return false;
}

static int MakeByteSizeString64(wchar_t * lpszBuf, size_t ccBuf, unsigned __int64 n64Byte)
{
  int nRet = 0;

  if (n64Byte < 1000ui64)
  {
    // < 1K
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%I64d B", n64Byte);
  }
  else if (n64Byte < 1024000ui64) // 1024 * 1000
  {
    // 1K <= n64Byte < 1M
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.1f KB", (double)n64Byte / 1024.0);
  }
  else if (n64Byte < 1048576000ui64) //  1024 * 1024 * 1000
  {
    // 1M <= n64Byte < 1G
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f MB", (double)n64Byte / 1048576.0); // 1024 * 1024
  }
  else if (n64Byte < 1073741824000ui64) //  1024 * 1024 * 1024 * 1000
  {
    // 1 G <= n64Byte < 1T
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f GB", (double)n64Byte / 1073741824.0); // 1024.0F * 1024.0F * 1024.0F
  }
  else
  {
    // n64Byte >= 1T
    nRet = swprintf(lpszBuf, ccBuf, 
      L"%.2f TB", (double)n64Byte / 1099511627776.0);
                  // 1024.0F * 1024.0F * 1024.0F * 1024.0F
  }

  return nRet;
}

void CCopyDialog::ShowPathFreeSpace(UString & strPath)
{
  bool bBadPath;
  UString strText;

  strText.Empty();

  bBadPath = false;
  strPath.Trim();
  for (; !IsDirectory(strPath); )
  {
    int n = strPath.ReverseFind(L'\\');
    if (n == -1)
    {
      bBadPath = true;
      break;
    }
    else
    {
      strPath.ReleaseBuf_SetEnd(n);
    }
  }
  if (!bBadPath)
  {
    unsigned __int64 n64FreeBytesAvailable;
    unsigned __int64 n64TotalNumberOfBytes;
    unsigned __int64 n64TotalNumberOfFreeBytes;

    if (GetDiskFreeSpaceExW(strPath, (PULARGE_INTEGER)&n64FreeBytesAvailable, 
      (PULARGE_INTEGER)&n64TotalNumberOfBytes, (PULARGE_INTEGER)&n64TotalNumberOfFreeBytes))
    {
      wchar_t szFreeBytes[1024];
      wchar_t szTotalBytes[1024];
      MakeByteSizeString64(szFreeBytes, 1024-4, n64TotalNumberOfFreeBytes);
      MakeByteSizeString64(szTotalBytes, 1024-4, n64TotalNumberOfBytes);
      int nLen = swprintf(strText.GetBuf(1024), 1024-4,  L"%s Free (Total: %s)", szFreeBytes, szTotalBytes);
      strText.ReleaseBuf_SetEnd(nLen);
    }
  }
  _freeSpace.SetText(strText);
}

bool CCopyDialog::OnCommand(int code, int itemID, LPARAM lParam)
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
