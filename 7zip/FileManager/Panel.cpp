// Panel.cpp

#include "StdAfx.h"

#include <Windowsx.h>

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Windows/Error.h"
#include "Windows/PropVariant.h"
#include "Windows/Shell.h"

#include "../PropID.h"

#include "Panel.h"
#include "RootFolder.h"
#include "FSFolder.h"
#include "FormatUtils.h"
#include "App.h"

#include "../UI/Common/CompressCall.h"
#include "../UI/Common/ArchiveName.h"

using namespace NWindows;

static const UINT_PTR kTimerID = 1;
static const UINT kTimerElapse = 1000;

// static const int kCreateFolderID = 101;

// static const UINT kFileChangeNotifyMessage = WM_APP;


extern HINSTANCE g_hInstance;
extern DWORD g_ComCtl32Version;

void CPanel::Release()
{
  // It's for unloading COM dll's: don't change it. 
  CloseOpenFolders();
  _sevenZipContextMenu.Release();
  _systemContextMenu.Release();
}

CPanel::~CPanel()
{
  CloseOpenFolders();
}

static LPCTSTR kClassName = TEXT("7-Zip::Panel");


LRESULT CPanel::Create(HWND mainWindow, HWND parentWindow, UINT id, int xPos, 
    const UString &currentFolderPrefix, CPanelCallback *panelCallback, CAppState *appState)
{
  _mainWindow = mainWindow;
  _processTimer = true;
  _processNotify = true;

  _panelCallback = panelCallback;
  _appState = appState;
  // _index = index;
  _baseID = id;
  _comboBoxID = _baseID + 3;
  _statusBarID = _comboBoxID + 1;
  _ListViewMode = 0;
  
  BindToPath(currentFolderPrefix);

  if (!CreateEx(0, kClassName, 0, WS_CHILD | WS_VISIBLE, 
      xPos, 0, 116, 260, 
      parentWindow, (HMENU)id, g_hInstance))
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
    case WM_DROPFILES:
      CompressDropFiles(HDROP(wParam));
      return 0;
  }
  return CWindow2::OnMessage(message, wParam, lParam);
}

static LRESULT APIENTRY ListViewSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  CWindow tempDialog(hwnd);
  CMyListView *w = (CMyListView *)(tempDialog.GetUserDataLongPtr());
  if (w == NULL)
    return 0;
  return w->OnMessage(message, wParam, lParam);
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
  else if (message == WM_SYSCHAR)
  {
    // For Alt+Enter Beep disabling
    UINT scanCode = (lParam >> 16) & 0xFF;
    UINT virtualKey = MapVirtualKey(scanCode, 1);
    if (virtualKey == VK_RETURN || virtualKey == VK_MULTIPLY || 
        virtualKey == VK_ADD || virtualKey == VK_SUBTRACT)
      return 0;
  }
  /*
  else if (message == WM_SYSKEYDOWN)
  {
    // return 0;
  }
  */
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
  else if (message == WM_SETFOCUS)
  {
    _panel->_lastFocusedIsList = true;
    _panel->_panelCallback->PanelWasFocused();
  }
  return CallWindowProc(_origWindowProc, *this, message, wParam, lParam); 
}

/*
static LRESULT APIENTRY ComboBoxSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  CWindow tempDialog(hwnd);
  CMyComboBox *w = (CMyComboBox *)(tempDialog.GetUserDataLongPtr());
  if (w == NULL)
    return 0;
  return w->OnMessage(message, wParam, lParam);
} 

LRESULT CMyComboBox::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  return CallWindowProc(_origWindowProc, *this, message, wParam, lParam); 
}
*/
static LRESULT APIENTRY ComboBoxEditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
  CWindow tempDialog(hwnd);
  CMyComboBoxEdit *w = (CMyComboBoxEdit *)(tempDialog.GetUserDataLongPtr());
  if (w == NULL)
    return 0;
  return w->OnMessage(message, wParam, lParam);
} 

LRESULT CMyComboBoxEdit::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  // See MSDN / Subclassing a Combo Box / Creating a Combo-box Toolbar
  switch (message) 
  { 
    case WM_SYSKEYDOWN: 
      switch (wParam) 
      { 
        case VK_F1: 
        case VK_F2: 
        {
          // check ALT
          if ((lParam & (1<<29)) == 0)
            break;
          bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
          bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
          bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
          if (alt && !ctrl && !shift)
          {
            _panel->_panelCallback->SetFocusToPath(wParam == VK_F1 ? 0 : 1);
            return 0; 
          }
          break; 
        }
      }
      break;
    case WM_KEYDOWN: 
      switch (wParam) 
      { 
        case VK_TAB: 
          // SendMessage(hwndMain, WM_ENTER, 0, 0); 
          _panel->SetFocusToList();
          return 0; 
        case VK_F9: 
        {
          bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
          bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
          bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
          if (!alt && !ctrl && !shift)
          {
            g_App.SwitchOnOffOnePanel();;
            return 0; 
          }
          break; 
        }
      }
      break;
    case WM_CHAR: 
      switch (wParam) 
      { 
        case VK_TAB: 
        case VK_ESCAPE: 
          return 0; 
      } 
  }
  return CallWindowProc(_origWindowProc, *this, message, wParam, lParam); 
}

bool CPanel::OnCreate(CREATESTRUCT *createStruct)
{
  // _virtualMode = false;
  // _sortIndex = 0;
  _sortID = kpidName;
  _ascending = true;
  _lastFocusedIsList = true;

  DWORD style = WS_CHILD | WS_VISIBLE; //  | WS_BORDER ; // | LVS_SHAREIMAGELISTS; //  | LVS_SHOWSELALWAYS;;

  style |= LVS_SHAREIMAGELISTS;
  // style  |= LVS_AUTOARRANGE;
  style  |= WS_CLIPCHILDREN;
  style |= WS_CLIPSIBLINGS;
  style |= WS_TABSTOP |  LVS_REPORT | LVS_EDITLABELS | LVS_SINGLESEL;

  /*
  if (_virtualMode)
    style |= LVS_OWNERDATA;
  */

  DWORD exStyle;
  exStyle = WS_EX_CLIENTEDGE;

  if (!_listView.CreateEx(exStyle, style, 0, 0, 116, 260, 
      HWND(*this), (HMENU)_baseID + 1, g_hInstance, NULL))
    return false;
  SetListViewMode(3);

  _listView.SetUserDataLongPtr(LONG_PTR(&_listView));
  _listView._panel = this;
  _listView._origWindowProc = (WNDPROC)_listView.SetLongPtr(GWLP_WNDPROC,
      LONG_PTR(ListViewSubclassProc));

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
  

  // Ensure that the common control DLL is loaded. 
  INITCOMMONCONTROLSEX icex;

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);

  TBBUTTON tbb [ ] = 
  {
    // {0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
    {VIEW_PARENTFOLDER, kParentFolderID, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
    // {0, 0, TBSTATE_ENABLED, BTNS_SEP, 0L, 0},
    // {VIEW_NEWFOLDER, kCreateFolderID, TBSTATE_ENABLED, BTNS_BUTTON, 0L, 0},
  };

  if (g_ComCtl32Version >= MAKELONG(71, 4))
  {
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    _headerReBar.Attach(::CreateWindowEx(WS_EX_TOOLWINDOW,
      REBARCLASSNAME,
      NULL, WS_VISIBLE | WS_BORDER | WS_CHILD | 
      WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
      | CCS_NODIVIDER  
      | CCS_NOPARENTALIGN 
      | CCS_TOP
      | RBS_VARHEIGHT 
      | RBS_BANDBORDERS
      ,0,0,0,0, HWND(*this), NULL, g_hInstance, NULL));
  }

  DWORD toolbarStyle =  WS_CHILD | WS_VISIBLE ;
  if (_headerReBar)
  {
    toolbarStyle |= 0
      // | WS_CLIPCHILDREN 
      // | WS_CLIPSIBLINGS 

      | TBSTYLE_TOOLTIPS
      | CCS_NODIVIDER
      | CCS_NORESIZE
      | TBSTYLE_FLAT
      ;
  }



  _headerToolBar.Attach(::CreateToolbarEx ((*this), toolbarStyle, 
      _baseID + 2, 11, 
      (HINSTANCE)HINST_COMMCTRL, 
      IDB_VIEW_SMALL_COLOR, 
      (LPCTBBUTTON)&tbb, sizeof(tbb) / sizeof(tbb[0]), 
      0, 0, 0, 0, sizeof (TBBUTTON)));

  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_USEREX_CLASSES;
  InitCommonControlsEx(&icex);
  
  _headerComboBox.CreateEx(0, WC_COMBOBOXEX, NULL,
    WS_BORDER | WS_VISIBLE |WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL,
      0, 0, 100, 20,
      ((_headerReBar != 0) ? HWND(*this) : _headerToolBar),
      (HMENU)(_comboBoxID),
      g_hInstance, NULL);
  _headerComboBox.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);

  /*
  _headerComboBox.SetUserDataLongPtr(LONG_PTR(&_headerComboBox));
  _headerComboBox._panel = this;
  _headerComboBox._origWindowProc = 
      (WNDPROC)_headerComboBox.SetLongPtr(GWLP_WNDPROC,
      LONG_PTR(ComboBoxSubclassProc));
  */
  _comboBoxEdit.Attach(_headerComboBox.GetEditControl());
  _comboBoxEdit.SetUserDataLongPtr(LONG_PTR(&_comboBoxEdit));
  _comboBoxEdit._panel = this;
  _comboBoxEdit._origWindowProc = 
      (WNDPROC)_comboBoxEdit.SetLongPtr(GWLP_WNDPROC,
      LONG_PTR(ComboBoxEditSubclassProc));


  if (_headerReBar)
  {
    REBARINFO     rbi;
    rbi.cbSize = sizeof(REBARINFO);  // Required when using this struct.
    rbi.fMask  = 0;
    rbi.himl   = (HIMAGELIST)NULL;
    _headerReBar.SetBarInfo(&rbi);
    
    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility. 
    // _headerToolBar.SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0); 
    SIZE size;
    _headerToolBar.GetMaxSize(&size);
    
    REBARBANDINFO rbBand;
    rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
    rbBand.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rbBand.fStyle = RBBS_NOGRIPPER;
    rbBand.cxMinChild = size.cx;
    rbBand.cyMinChild = size.cy;
    rbBand.cyChild = size.cy;
    rbBand.cx = size.cx;
    rbBand.hwndChild  = _headerToolBar;
    _headerReBar.InsertBand(-1, &rbBand);

    RECT rc;
    ::GetWindowRect(_headerComboBox, &rc);
    rbBand.cxMinChild = 30;
    rbBand.cyMinChild = rc.bottom - rc.top;
    rbBand.cx = 1000;
    rbBand.hwndChild  = _headerComboBox;
    _headerReBar.InsertBand(-1, &rbBand);
    // _headerReBar.MaximizeBand(1, false);
  }

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

  // InitListCtrl();
  RefreshListCtrl();
  RefreshStatusBar();
  ::DragAcceptFiles(HWND(*this), TRUE);  
  return true;
}

void CPanel::OnDestroy()
{
  ::DragAcceptFiles(HWND(*this), FALSE);  
  SaveListViewInfo();
  CWindow2::OnDestroy();
}

void CPanel::ChangeWindowSize(int xSize, int ySize) 
{
  int kHeaderSize;
  int kStatusBarSize;
  // int kStatusBar2Size;
  RECT rect;
  if (_headerReBar)
    _headerReBar.GetWindowRect(&rect);
  else
    _headerToolBar.GetWindowRect(&rect);

  kHeaderSize = rect.bottom - rect.top;

  _statusBar.GetWindowRect(&rect);
  kStatusBarSize = rect.bottom - rect.top;
  
  // _statusBar2.GetWindowRect(&rect);
  // kStatusBar2Size = rect.bottom - rect.top;
 
  int yListViewSize = MyMax(ySize - kHeaderSize - kStatusBarSize, 0);
  const int kStartXPos = 32;
  if (_headerReBar)
  {
  }
  else
  {
    _headerToolBar.Move(0, 0, xSize, 0);
    _headerComboBox.Move(kStartXPos, 2, 
        MyMax(xSize - kStartXPos - 10, kStartXPos), 0);
  }
  _listView.Move(0, kHeaderSize, xSize, yListViewSize);
  _statusBar.Move(0, kHeaderSize + yListViewSize, xSize, kStatusBarSize);
  // _statusBar2.MoveWindow(0, kHeaderSize + yListViewSize + kStatusBarSize, xSize, kStatusBar2Size);
  // _statusBar.MoveWindow(0, 100, xSize, kStatusBarSize);
  // _statusBar2.MoveWindow(0, 200, xSize, kStatusBar2Size);
}

bool CPanel::OnSize(WPARAM wParam, int xSize, int ySize) 
{
  if (_headerReBar)
    _headerReBar.Move(0, 0, xSize, 0);
  ChangeWindowSize(xSize, ySize);
  return true;
}

bool CPanel::OnNotifyReBar(LPNMHDR header, LRESULT &result)
{
  switch(header->code)
  {
    case RBN_HEIGHTCHANGE:
    {
      RECT rect;
      GetWindowRect(&rect);
      ChangeWindowSize(rect.right - rect.left, rect.bottom - rect.top);
      return false;
    }
  }
  return false;
}

bool CPanel::OnNotify(UINT controlID, LPNMHDR header, LRESULT &result)
{
  if (!_processNotify)
    return false;
  if (header->hwndFrom == _headerComboBox)
    return OnNotifyComboBox(header, result);
  else if (header->hwndFrom == _headerReBar)
    return OnNotifyReBar(header, result);
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

void CPanel::SetFocusToList()
{
  _listView.SetFocus();
  // SetCurrentPathText();
}

void CPanel::SetFocusToLastRememberedItem()
{
  if (_lastFocusedIsList)
    SetFocusToList();
  else
    _headerComboBox.SetFocus();
}

CSysString CPanel::GetFileType(UINT32 index)
{
  return TEXT("Test type");
}

UString CPanel::GetFolderTypeID() const
{
  CMyComPtr<IFolderGetTypeID> folderGetTypeID;
  if(_folder.QueryInterface(IID_IFolderGetTypeID, &folderGetTypeID) != S_OK)
    return L"";
  CMyComBSTR typeID;
  folderGetTypeID->GetTypeID(&typeID);
  return typeID;
}

bool CPanel::IsRootFolder() const
{
  return (GetFolderTypeID() == L"RootFolder");
}

bool CPanel::IsFSFolder() const
{
  return (GetFolderTypeID() == L"FSFolder");
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

void CPanel::CompressDropFiles(HDROP dr)
{
  NShell::CDrop drop(true);
  drop.Attach(dr);
  CSysStringVector fileNames;
  drop.QueryFileNames(fileNames);
  if (fileNames.Size() == 0)
    return;
  UStringVector fileNamesUnicode;
  for (int i = 0; i < fileNames.Size(); i++)
    fileNamesUnicode.Add(GetUnicodeString(fileNames[i]));
  const UString &archiveName = CreateArchiveName(
    fileNamesUnicode.Front(), (fileNamesUnicode.Size() > 1), false);
  UString currentDirectory;
  if (IsFSFolder())
  {
    CompressFiles(_currentFolderPrefix + archiveName, fileNamesUnicode, 
      false, // email
      true // showDialog
      );
  }
  else
  {
    _panelCallback->OnCopy(fileNamesUnicode, false, true);
    /*
    if (!NFile::NDirectory::GetOnlyDirPrefix(fileNames.Front(), currentDirectory))
      return;
    */
  }
}