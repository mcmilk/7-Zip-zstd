// Panel.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Panel.h"
#include "Windows/Error.h"
#include "Windows/PropVariant.h"
#include "Windows/FileDir.h"

#include "Common/StringConvert.h"

#include "../Archiver/Format/Common/IArchiveHandler.h"
#include "RootFolder.h"

#include "FSFolder.h"

#include "FormatUtils.h"

#include <Windowsx.h>

using namespace NWindows;
using namespace NFile;
using namespace NFind;

static const UINT_PTR kTimerID = 1;
static const UINT kTimerElapse = 1000;

static const kParentFolderID = 100;
// static const kCreateFolderID = 101;

// static const UINT kFileChangeNotifyMessage = WM_APP;


extern HINSTANCE g_hInstance;


CPanel::~CPanel()
{
  CloseOpenFolders();
}

static LPCTSTR kClassName = TEXT("7-Zip::Panel");

LRESULT CPanel::Create(HWND parrentWindow, int index, UINT id, int xPos, 
    CSysString &currentFolderPrefix, CPanelCallback *panelCallback, CAppState *appState)
{
  _processTimer = true;
  _processNotify = true;

  _panelCallback = panelCallback;
  _appState = appState;
  _index = index;
  _baseID = id;
  _comboBoxID = _baseID + 3;
  _statusBarID = _comboBoxID + 1;
  _ListViewMode = 0;
  
  SetToRootFolder();
  CComPtr<IFolderFolder> newFolder;

  CSysString path = currentFolderPrefix;
  CFileInfo fileInfo;
  while(!path.IsEmpty())
  {
    if (FindFile(path, fileInfo))
      break;
    int pos = path.ReverseFind('\\');
    if (pos < 0)
      path.Empty();
    else
      path = path.Left(pos);
  }
  if (path.IsEmpty())
  {
    if (_folder->BindToFolder(GetUnicodeString(currentFolderPrefix), 
      &newFolder) == S_OK)
    {
      _folder = newFolder;
      SetCurrentPathText();
    }
  }
  else
  {
    if (fileInfo.IsDirectory())
    {
      NName::NormalizeDirPathPrefix(path);
      if (_folder->BindToFolder(GetUnicodeString(path), &newFolder) == S_OK)
      {
        _folder = newFolder;
        SetCurrentPathText();
      }
    }
    else
    {
      CSysString dirPrefix;
      if (!NDirectory::GetOnlyDirPrefix(path, dirPrefix))
        dirPrefix.Empty();
      if (_folder->BindToFolder(GetUnicodeString(dirPrefix), &newFolder) == S_OK)
      {
        _folder = newFolder;
        SetCurrentPathText();
        CSysString fileName;
        if (NDirectory::GetOnlyName(path, fileName))
        {
          OpenItemAsArchive(GetUnicodeString(fileName));
          SetCurrentPathText();
        }
      }
    }
  }

  if (!CreateEx(0, kClassName, 0, WS_CHILD | WS_VISIBLE, 
      xPos, 0, 116, 260, 
      parrentWindow, (HMENU)id, g_hInstance))
    return E_FAIL;
  
  return S_OK;
}

LRESULT CPanel::OnMessage(UINT message, UINT wParam, LPARAM lParam)
{
  switch(message)
  {
    case kShiftSelectMessage:
      OnShiftSelectMessage();
      return 0;
    case kReLoadMessage:
      OnReload();
      return 0;
    case kSetFocusToListView:
      _listView.SetFocus();
      return 0;
    case kOpenItemChanged:
      return OnOpenItemChanged(lParam);
    case kRefreshStatusBar:
      OnRefreshStatusBar();
      return 0;
    case WM_TIMER:
      OnTimer();
      return 0;
    case WM_CONTEXTMENU:
      if (OnContextMenu(HANDLE(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        return 0;
  }
  return CWindow2::OnMessage(message, wParam, lParam);
}

LRESULT APIENTRY EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  CWindow tempDialog(hwnd);
  CMyListView *dialog = (CMyListView *)(tempDialog.GetUserDataLongPtr());
  if (dialog == NULL)
    return 0;
  return dialog->OnMessage(message, wParam, lParam);
} 

LRESULT CMyListView::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_CHAR)
  {
    UINT scanCode = (lParam >> 16) & 0xFF;
    bool extended = ((lParam & 0x1000000) != 0);
    UINT virtualKey = MapVirtualKey(scanCode, 1);
    if (virtualKey == VK_MULTIPLY || virtualKey == VK_ADD ||
        virtualKey == VK_SUBTRACT)
      return 0;
    if ((wParam == '/' && extended)
        || wParam == '\\' || wParam == '/')
    {
      _panel->OpenDrivesFolder();
      return 0;
    }
  }
  else if (message == WM_KEYDOWN)
  {
    bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
    bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool leftCtrl = (::GetKeyState(VK_LCONTROL) & 0x8000) != 0;
    bool RightCtrl = (::GetKeyState(VK_RCONTROL) & 0x8000) != 0;
    bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    switch(wParam)
    {
      case VK_RETURN:
      {
        if (shift && !alt && !ctrl)
        {
          _panel->OpenSelectedItems(false);
          return 0;
        }
        break;
      }
      case VK_NEXT:
      {
        if (ctrl && !alt && !shift)
        {
          _panel->OpenFocusedItemAsInternal();
          return 0;
        }
        break;
      }
      case VK_PRIOR:
      if (ctrl && !alt && !shift)
      {
        _panel->OpenParentFolder();
        return 0;
      }
    }
  }

  return CallWindowProc(_origWindowProc, *this, message, wParam, lParam); 
}


bool CPanel::OnCreate(CREATESTRUCT *createStruct)
{
  // _sortIndex = 0;
  _sortID = kpidName;
  _ascending = true;

  DWORD style = WS_CHILD | WS_VISIBLE; //  | WS_BORDER ; // | LVS_SHAREIMAGELISTS; //  | LVS_SHOWSELALWAYS;;

  style |= LVS_SHAREIMAGELISTS;
  // style  |= LVS_AUTOARRANGE;
  
  style  |= WS_CLIPCHILDREN;
  
  style |= WS_CLIPSIBLINGS;

  style |= WS_TABSTOP |  LVS_REPORT | LVS_EDITLABELS | LVS_SINGLESEL;
  DWORD exStyle;
  exStyle = WS_EX_CLIENTEDGE;

  if (!_listView.CreateEx(exStyle, style, 0, 0, 116, 260, 
      HWND(*this), (HMENU)_baseID + 1, g_hInstance, NULL))
    return false;
  SetListViewMode(3);

  _listView.SetUserDataLongPtr(LONG_PTR(&_listView));
  _listView._panel = this;
  _listView._origWindowProc = (WNDPROC)_listView.SetLongPtr(GWLP_WNDPROC,
      LONG_PTR(EditSubclassProc));

  SHFILEINFO shellInfo;
  HIMAGELIST imageList = (HIMAGELIST)SHGetFileInfo(TEXT(""), 
      FILE_ATTRIBUTE_NORMAL |
      FILE_ATTRIBUTE_DIRECTORY, 
      &shellInfo, sizeof(shellInfo), 
      SHGFI_USEFILEATTRIBUTES | 
      SHGFI_SYSICONINDEX |
      SHGFI_SMALLICON
      );
  _listView.SetImageList(imageList, LVSIL_SMALL);
  imageList = (HIMAGELIST)SHGetFileInfo(TEXT(""), 
      FILE_ATTRIBUTE_NORMAL |
      FILE_ATTRIBUTE_DIRECTORY, 
      &shellInfo, sizeof(shellInfo), 
      SHGFI_USEFILEATTRIBUTES | 
      SHGFI_SYSICONINDEX |
      SHGFI_ICON
      );
  _listView.SetImageList(imageList, LVSIL_NORMAL);

  DWORD extendedStyle = _listView.GetExtendedListViewStyle();
  extendedStyle |= LVS_EX_HEADERDRAGDROP; // Version 4.70
  _listView.SetExtendedListViewStyle(extendedStyle);

  _listView.Show(SW_SHOW);
  _listView.InvalidateRect(NULL, true);
  _listView.Update();
  
  // InitListCtrl();
  RefreshListCtrl();

  // Ensure that the common control DLL is loaded. 
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);

  /*
  _headerToolBar.CreateEx(0, TOOLBARCLASSNAME, NULL,
      WS_VISIBLE | WS_CHILD, // | CCS_NODIVIDER | CCS_TOP
                0, 0, 0, 0, // set size in WM_SIZE message 
                (*this),  // parent window 
                (HMENU)(_baseID + 1), // edit control ID 
                g_hInstance, NULL);
  */

  // Toolbar buttons used to create the first 4 buttons.
  TBBUTTON tbb [ ] = 
  {
    // {0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
    {VIEW_PARENTFOLDER, kParentFolderID, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
    // {0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
    // {VIEW_NEWFOLDER, kCreateFolderID, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
  };

  _headerToolBar.Attach(::CreateToolbarEx ((*this), 
    WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS, //  | TBSTYLE_FLAT 
      _baseID + 2, 11, 
      (HINSTANCE)HINST_COMMCTRL, IDB_VIEW_SMALL_COLOR, 
      (LPCTBBUTTON)&tbb, sizeof(tbb) / sizeof(tbb[0]), 
      0, 0, 100, 30, sizeof (TBBUTTON)));


  // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
  // backward compatibility. 
  // _headerToolBar.SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_USEREX_CLASSES;
  InitCommonControlsEx(&icex);
  
  _headerComboBox.CreateEx(0, WC_COMBOBOXEX, NULL,
      WS_BORDER | WS_VISIBLE |WS_CHILD | CBS_DROPDOWN,
      0, 0, 0, 400,
      _headerToolBar, 
      (HMENU)(_comboBoxID),
      g_hInstance, NULL);

  _statusBar.Create(WS_CHILD | WS_VISIBLE, TEXT("Statuys"), (*this), _statusBarID);
  // _statusBar2.Create(WS_CHILD | WS_VISIBLE, TEXT("Statuys"), (*this), _statusBarID + 1);

  int sizes[] = {150, 200, 250, -1};
  _statusBar.SetParts(4, sizes);
  // _statusBar2.SetParts(5, sizes);


  /*
  RECT rect;
  GetClientRect(&rect);
  OnSize(0, rect.right - rect.left, rect.top - rect.bottom);
  */

  SetTimer(kTimerID, kTimerElapse);

  SetCurrentPathText();

  RefreshStatusBar();

  return true;
}

void CPanel::OnDestroy()
{
  SaveListViewInfo();
  CWindow2::OnDestroy();
}

bool CPanel::OnSize(WPARAM wParam, int xSize, int ySize) 
{ 
  int kHeaderSize;
  int kStatusBarSize;
  // int kStatusBar2Size;
  RECT rect;
  _headerToolBar.GetWindowRect(&rect);
  kHeaderSize = rect.bottom - rect.top;
  _statusBar.GetWindowRect(&rect);
  kStatusBarSize = rect.bottom - rect.top;
  
  // _statusBar2.GetWindowRect(&rect);
  // kStatusBar2Size = rect.bottom - rect.top;
 
  int yListViewSize = MyMax(ySize - kHeaderSize - kStatusBarSize, 0);
  _headerToolBar.Move(0, 0, xSize, 0);
  const int kStartXPos = 32;
  _headerComboBox.Move(kStartXPos, 2, 
      MyMax(xSize - kStartXPos - 10, kStartXPos), 0);
  _listView.Move(0, kHeaderSize, xSize, yListViewSize);
  _statusBar.Move(0, kHeaderSize + yListViewSize, xSize, kStatusBarSize);
  // _statusBar2.MoveWindow(0, kHeaderSize + yListViewSize + kStatusBarSize, xSize, kStatusBar2Size);
  // _statusBar.MoveWindow(0, 100, xSize, kStatusBarSize);
  // _statusBar2.MoveWindow(0, 200, xSize, kStatusBar2Size);
  return true;
}

bool CPanel::OnNotify(UINT controlID, LPNMHDR header, LRESULT &result)
{
  if (!_processNotify)
    return false;
  if (header->hwndFrom == _headerComboBox)
  {
    return OnNotifyComboBox(header, result);
  }
  // if (header->hwndFrom == _listView)
  else if (header->hwndFrom == _listView)
    return OnNotifyList(header, result);
  else if (::GetParent(header->hwndFrom) == _listView && 
      header->code == NM_RCLICK)
    return OnRightClick((LPNMITEMACTIVATE)header, result);
  return false;
}

bool CPanel::OnCommand(int code, int itemID, LPARAM lParam, LRESULT &result)
{
  if (itemID == kParentFolderID)
  {
    OpenParentFolder();
    result = 0;
    return true;
  }
  /*
  if (itemID == kCreateFolderID)
  {
    CreateFolder();
    result = 0;
    return true;
  }
  */
  if (itemID == _comboBoxID)
  {
    OnComboBoxCommand(code, lParam);
  }
  return CWindow2::OnCommand(code, itemID, lParam, result);
}

void CPanel::MessageBox(LPCWSTR message, LPCWSTR caption)
  { ::MessageBoxW(HWND(*this), message, caption, MB_OK | MB_ICONSTOP); }
void CPanel::MessageBox(LPCWSTR message)
  { MessageBox(message, L"7-Zip"); }
void CPanel::MessageBoxMyError(LPCWSTR message)
  { MessageBox(message, L"Error"); }
void CPanel::MessageBoxError(HRESULT errorCode, LPCWSTR caption)
  { MessageBox(GetUnicodeString(NError::MyFormatMessage(errorCode)), caption); }
void CPanel::MessageBoxLastError(LPCWSTR caption)
  { MessageBoxError(::GetLastError(), caption); }
void CPanel::MessageBoxLastError()
  { MessageBoxLastError(L"Error"); }


void CPanel::SetFocus()
{
  HWND a = _listView.SetFocus();
  SetCurrentPathText();
}


CSysString CPanel::GetFileType(UINT32 index)
{
  return TEXT("Test type");
}


bool CPanel::IsFSFolder() const
{
  CComPtr<IFolderGetTypeID> folderGetTypeID;
  if(_folder.QueryInterface(&folderGetTypeID) != S_OK)
    return false;
  CComBSTR typeID;
  folderGetTypeID->GetTypeID(&typeID);
  return (UString(typeID) == L"FSFolder");
}

static DWORD kStyles[4] = { LVS_ICON, LVS_SMALLICON, LVS_LIST, LVS_REPORT };

void CPanel::SetListViewMode(UINT32 index)
{
  if (index >= 4)
    return;
  _ListViewMode = index;
  DWORD oldStyle = _listView.GetStyle();
  DWORD newStyle = kStyles[index];
  if ((oldStyle & LVS_TYPEMASK) != newStyle)
    _listView.SetStyle((oldStyle & ~LVS_TYPEMASK) | newStyle);
  // RefreshListCtrlSaveFocused();
}

void CPanel::RefreshStatusBar()
{
  PostMessage(kRefreshStatusBar);
}

