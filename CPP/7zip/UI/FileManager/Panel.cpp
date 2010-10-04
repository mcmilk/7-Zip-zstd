// Panel.cpp

#include "StdAfx.h"

#include <Windowsx.h>

#include "Common/Defs.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/PropVariant.h"
#include "Windows/Thread.h"

#include "../../PropID.h"

#include "resource.h"
#include "../GUI/ExtractRes.h"

#include "../Common/ArchiveName.h"
#include "../Common/CompressCall.h"

#include "../Agent/IFolderArchive.h"

#include "App.h"
#include "ExtractCallback.h"
#include "FSFolder.h"
#include "FormatUtils.h"
#include "Panel.h"
#include "RootFolder.h"


using namespace NWindows;
using namespace NControl;

#ifndef _UNICODE
extern bool g_IsNT;
#endif

static const UINT_PTR kTimerID = 1;
static const UINT kTimerElapse = 1000;

static DWORD kStyles[4] = { LVS_ICON, LVS_SMALLICON, LVS_LIST, LVS_REPORT };

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

HWND CPanel::GetParent()
{
  HWND h = CWindow2::GetParent();
  return (h == 0) ? _mainWindow : h;
}

static LPCWSTR kClassName = L"7-Zip::Panel";


HRESULT CPanel::Create(HWND mainWindow, HWND parentWindow, UINT id,
    const UString &currentFolderPrefix,
    const UString &arcFormat,
    CPanelCallback *panelCallback, CAppState *appState,
    bool &archiveIsOpened, bool &encrypted)
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

  UString cfp = currentFolderPrefix;

  if (!currentFolderPrefix.IsEmpty())
    if (currentFolderPrefix[0] == L'.')
      if (!NFile::NDirectory::MyGetFullPathName(currentFolderPrefix, cfp))
        cfp = currentFolderPrefix;
  RINOK(BindToPath(cfp, arcFormat, archiveIsOpened, encrypted));

  if (!CreateEx(0, kClassName, 0, WS_CHILD | WS_VISIBLE,
      0, 0, _xSize, 260,
      parentWindow, (HMENU)(UINT_PTR)id, g_hInstance))
    return E_FAIL;
  return S_OK;
}

LRESULT CPanel::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case kShiftSelectMessage:
      OnShiftSelectMessage();
      return 0;
    case kReLoadMessage:
      RefreshListCtrl(_selectedState);
      return 0;
    case kSetFocusToListView:
      _listView.SetFocus();
      return 0;
    case kOpenItemChanged:
      return OnOpenItemChanged(lParam);
    case kRefreshStatusBar:
      OnRefreshStatusBar();
      return 0;
    case kRefreshHeaderComboBox:
      LoadFullPathAndShow();
      return 0;
    case WM_TIMER:
      OnTimer();
      return 0;
    case WM_CONTEXTMENU:
      if (OnContextMenu(HANDLE(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
        return 0;
      break;
    /*
    case WM_DROPFILES:
      CompressDropFiles(HDROP(wParam));
      return 0;
    */
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
    UINT scanCode = (UINT)((lParam >> 16) & 0xFF);
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
    UINT scanCode = (UINT)(lParam >> 16) & 0xFF;
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
    // bool leftCtrl = (::GetKeyState(VK_LCONTROL) & 0x8000) != 0;
    // bool RightCtrl = (::GetKeyState(VK_RCONTROL) & 0x8000) != 0;
    bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
    switch(wParam)
    {
      /*
      case VK_RETURN:
      {
        if (shift && !alt && !ctrl)
        {
          _panel->OpenSelectedItems(false);
          return 0;
        }
        break;
      }
      */
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
  #ifdef UNDER_CE
  else if (message == WM_KEYUP)
  {
    if (wParam == VK_F2) // it's VK_TSOFT2
    {
      // Activate Menu
      ::PostMessage(g_HWND, WM_SYSCOMMAND, SC_KEYMENU, 0);
      return 0;
    }
  }
  #endif
  else if (message == WM_SETFOCUS)
  {
    _panel->_lastFocusedIsList = true;
    _panel->_panelCallback->PanelWasFocused();
  }
  #ifndef _UNICODE
  if (g_IsNT)
    return CallWindowProcW(_origWindowProc, *this, message, wParam, lParam);
  else
  #endif
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
  #ifndef _UNICODE
  if (g_IsNT)
    return CallWindowProcW(_origWindowProc, *this, message, wParam, lParam);
  else
  #endif
    return CallWindowProc(_origWindowProc, *this, message, wParam, lParam);
}

bool CPanel::OnCreate(CREATESTRUCT * /* createStruct */)
{
  // _virtualMode = false;
  // _sortIndex = 0;
  _sortID = kpidName;
  _ascending = true;
  _lastFocusedIsList = true;

  DWORD style = WS_CHILD | WS_VISIBLE; //  | WS_BORDER ; // | LVS_SHAREIMAGELISTS; //  | LVS_SHOWSELALWAYS;;

  style |= LVS_SHAREIMAGELISTS;
  // style  |= LVS_AUTOARRANGE;
  style |= WS_CLIPCHILDREN;
  style |= WS_CLIPSIBLINGS;

  const UInt32 kNumListModes = sizeof(kStyles) / sizeof(kStyles[0]);
  if (_ListViewMode >= kNumListModes)
    _ListViewMode = kNumListModes - 1;

  style |= kStyles[_ListViewMode]
    | WS_TABSTOP
    | LVS_EDITLABELS;
  if (_mySelectMode)
    style |= LVS_SINGLESEL;

  /*
  if (_virtualMode)
    style |= LVS_OWNERDATA;
  */

  DWORD exStyle;
  exStyle = WS_EX_CLIENTEDGE;

  if (!_listView.CreateEx(exStyle, style, 0, 0, 116, 260,
      HWND(*this), (HMENU)(UINT_PTR)(_baseID + 1), g_hInstance, NULL))
    return false;

  #ifndef UNDER_CE
  _listView.SetUnicodeFormat(true);
  #endif

  _listView.SetUserDataLongPtr(LONG_PTR(&_listView));
  _listView._panel = this;

   #ifndef _UNICODE
   if(g_IsNT)
     _listView._origWindowProc =
      (WNDPROC)_listView.SetLongPtrW(GWLP_WNDPROC, LONG_PTR(ListViewSubclassProc));
   else
   #endif
     _listView._origWindowProc =
      (WNDPROC)_listView.SetLongPtr(GWLP_WNDPROC, LONG_PTR(ListViewSubclassProc));

  _listView.SetImageList(GetSysImageList(true), LVSIL_SMALL);
  _listView.SetImageList(GetSysImageList(false), LVSIL_NORMAL);

  // _exStyle |= LVS_EX_HEADERDRAGDROP;
  // DWORD extendedStyle = _listView.GetExtendedListViewStyle();
  // extendedStyle |= _exStyle;
  //  _listView.SetExtendedListViewStyle(extendedStyle);
  SetExtendedStyle();

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

  #ifndef UNDER_CE
  if (g_ComCtl32Version >= MAKELONG(71, 4))
  #endif
  {
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    _headerReBar.Attach(::CreateWindowEx(WS_EX_TOOLWINDOW,
      REBARCLASSNAME,
      NULL, WS_VISIBLE | WS_BORDER | WS_CHILD |
      WS_CLIPCHILDREN | WS_CLIPSIBLINGS
      | CCS_NODIVIDER
      // | CCS_NOPARENTALIGN
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

  #ifndef UNDER_CE
  // Load ComboBoxEx class
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_USEREX_CLASSES;
  InitCommonControlsEx(&icex);
  #endif
  
  _headerComboBox.CreateEx(0,
      #ifdef UNDER_CE
      WC_COMBOBOXW
      #else
      WC_COMBOBOXEXW
      #endif
      , NULL,
    WS_BORDER | WS_VISIBLE |WS_CHILD | CBS_DROPDOWN | CBS_AUTOHSCROLL,
      0, 0, 100, 520,
      ((_headerReBar == 0) ? HWND(*this) : _headerToolBar),
      (HMENU)(UINT_PTR)(_comboBoxID),
      g_hInstance, NULL);
  #ifndef UNDER_CE
  _headerComboBox.SetUnicodeFormat(true);

  _headerComboBox.SetImageList(GetSysImageList(true));

  _headerComboBox.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);

  /*
  _headerComboBox.SetUserDataLongPtr(LONG_PTR(&_headerComboBox));
  _headerComboBox._panel = this;
  _headerComboBox._origWindowProc =
      (WNDPROC)_headerComboBox.SetLongPtr(GWLP_WNDPROC,
      LONG_PTR(ComboBoxSubclassProc));
  */
  _comboBoxEdit.Attach(_headerComboBox.GetEditControl());

  // _comboBoxEdit.SendMessage(CCM_SETUNICODEFORMAT, (WPARAM)(BOOL)TRUE, 0);

  _comboBoxEdit.SetUserDataLongPtr(LONG_PTR(&_comboBoxEdit));
  _comboBoxEdit._panel = this;
   #ifndef _UNICODE
   if(g_IsNT)
     _comboBoxEdit._origWindowProc =
      (WNDPROC)_comboBoxEdit.SetLongPtrW(GWLP_WNDPROC, LONG_PTR(ComboBoxEditSubclassProc));
   else
   #endif
     _comboBoxEdit._origWindowProc =
      (WNDPROC)_comboBoxEdit.SetLongPtr(GWLP_WNDPROC, LONG_PTR(ComboBoxEditSubclassProc));

  #endif

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
    rbBand.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
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

  _statusBar.Create(WS_CHILD | WS_VISIBLE, L"Status", (*this), _statusBarID);
  // _statusBar2.Create(WS_CHILD | WS_VISIBLE, L"Status", (*this), _statusBarID + 1);

  int sizes[] = {150, 250, 350, -1};
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
  
  return true;
}

void CPanel::OnDestroy()
{
  SaveListViewInfo();
  CWindow2::OnDestroy();
}

void CPanel::ChangeWindowSize(int xSize, int ySize)
{
  if ((HWND)*this == 0)
    return;
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

bool CPanel::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  if ((HWND)*this == 0)
    return true;
  if (_headerReBar)
    _headerReBar.Move(0, 0, xSize, 0);
  ChangeWindowSize(xSize, ySize);
  return true;
}

bool CPanel::OnNotifyReBar(LPNMHDR header, LRESULT & /* result */)
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

bool CPanel::OnNotify(UINT /* controlID */, LPNMHDR header, LRESULT &result)
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
    return OnRightClick((MY_NMLISTVIEW_NMITEMACTIVATE *)header, result);
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
    if (OnComboBoxCommand(code, lParam, result))
      return true;
  }
  return CWindow2::OnCommand(code, itemID, lParam, result);
}

void CPanel::MessageBoxInfo(LPCWSTR message, LPCWSTR caption)
  { ::MessageBoxW(HWND(*this), message, caption, MB_OK); }
void CPanel::MessageBox(LPCWSTR message, LPCWSTR caption)
  { ::MessageBoxW(HWND(*this), message, caption, MB_OK | MB_ICONSTOP); }
void CPanel::MessageBox(LPCWSTR message)
  { MessageBox(message, L"7-Zip"); }
void CPanel::MessageBoxMyError(LPCWSTR message)
  { MessageBox(message, L"Error"); }


void CPanel::MessageBoxError(HRESULT errorCode, LPCWSTR caption)
{
  MessageBox(HResultToMessage(errorCode), caption);
}

void CPanel::MessageBoxError(HRESULT errorCode)
  { MessageBoxError(errorCode, L"7-Zip"); }
void CPanel::MessageBoxLastError(LPCWSTR caption)
  { MessageBoxError(::GetLastError(), caption); }
void CPanel::MessageBoxLastError()
  { MessageBoxLastError(L"Error"); }

void CPanel::MessageBoxErrorLang(UINT resourceID, UInt32 langID)
  { MessageBox(LangString(resourceID, langID)); }


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

UString CPanel::GetFolderTypeID() const
{
  NCOM::CPropVariant prop;
  if (_folder->GetFolderProperty(kpidType, &prop) == S_OK)
    if (prop.vt == VT_BSTR)
      return (const wchar_t *)prop.bstrVal;
  return L"";
}

bool CPanel::IsFolderTypeEqTo(const wchar_t *s) const
{
  return GetFolderTypeID() == s;
}

bool CPanel::IsRootFolder() const { return IsFolderTypeEqTo(L"RootFolder"); }
bool CPanel::IsFSFolder() const { return IsFolderTypeEqTo(L"FSFolder"); }
bool CPanel::IsFSDrivesFolder() const { return IsFolderTypeEqTo(L"FSDrives"); }
bool CPanel::IsArcFolder() const
{
  UString s = GetFolderTypeID();
  return s.Left(5) == L"7-Zip";
}

UString CPanel::GetFsPath() const
{
  if (IsFSDrivesFolder() && !IsDeviceDrivesPrefix())
    return UString();
  return _currentFolderPrefix;
}

UString CPanel::GetDriveOrNetworkPrefix() const
{
  if (!IsFSFolder())
    return UString();
  UString drive = GetFsPath();
  if (drive.Length() < 3)
    return UString();
  if (drive[0] == L'\\' && drive[1] == L'\\')
  {
    // if network
    int pos = drive.Find(L'\\', 2);
    if (pos < 0)
      return UString();
    pos = drive.Find(L'\\', pos + 1);
    if (pos < 0)
      return UString();
    return drive.Left(pos + 1);
  }
  if (drive[1] != L':' || drive[2] != L'\\')
    return UString();
  return drive.Left(3);
}

bool CPanel::DoesItSupportOperations() const
{
  CMyComPtr<IFolderOperations> folderOperations;
  return _folder.QueryInterface(IID_IFolderOperations, &folderOperations) == S_OK;
}

void CPanel::SetListViewMode(UInt32 index)
{
  if (index >= 4)
    return;
  _ListViewMode = index;
  DWORD oldStyle = (DWORD)_listView.GetStyle();
  DWORD newStyle = kStyles[index];
  if ((oldStyle & LVS_TYPEMASK) != newStyle)
    _listView.SetStyle((oldStyle & ~LVS_TYPEMASK) | newStyle);
  // RefreshListCtrlSaveFocused();
}

void CPanel::ChangeFlatMode()
{
  _flatMode = !_flatMode;
  if (_parentFolders.Size() > 0)
    _flatModeForArc = _flatMode;
  else
    _flatModeForDisk = _flatMode;
  RefreshListCtrlSaveFocused();
}


void CPanel::RefreshStatusBar()
{
  PostMessage(kRefreshStatusBar);
}

void CPanel::AddToArchive()
{
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  if (!IsFsOrDrivesFolder())
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }
  if (indices.Size() == 0)
  {
    MessageBoxErrorLang(IDS_SELECT_FILES, 0x03020A03);
    return;
  }
  UStringVector names;

  UString curPrefix = _currentFolderPrefix;
  UString destCurDirPrefix = _currentFolderPrefix;
  if (IsFSDrivesFolder())
  {
    destCurDirPrefix = ROOT_FS_FOLDER;
    if (!IsDeviceDrivesPrefix())
      curPrefix.Empty();
  }

  for (int i = 0; i < indices.Size(); i++)
    names.Add(curPrefix + GetItemRelPath(indices[i]));
  const UString archiveName = CreateArchiveName(names.Front(), (names.Size() > 1), false);
  HRESULT res = CompressFiles(destCurDirPrefix, archiveName, L"", names, false, true, false);
  if (res != S_OK)
  {
    if (destCurDirPrefix.Length() >= MAX_PATH)
      MessageBoxErrorLang(IDS_MESSAGE_UNSUPPORTED_OPERATION_FOR_LONG_PATH_FOLDER, 0x03020A01);
  }
  // KillSelection();
}

static UString GetSubFolderNameForExtract(const UString &archiveName)
{
  int slashPos = archiveName.ReverseFind(WCHAR_PATH_SEPARATOR);
  int dotPos = archiveName.ReverseFind(L'.');
  if (dotPos < 0 || slashPos > dotPos)
    return archiveName + UString(L"~");
  UString res = archiveName.Left(dotPos);
  res.TrimRight();
  return res;
}

void CPanel::GetFilePaths(const CRecordVector<UInt32> &indices, UStringVector &paths)
{
  for (int i = 0; i < indices.Size(); i++)
  {
    int index = indices[i];
    if (IsItemFolder(index))
    {
      paths.Clear();
      break;
    }
    paths.Add(GetItemFullPath(index));
  }
  if (paths.Size() == 0)
  {
    MessageBoxErrorLang(IDS_SELECT_FILES, 0x03020A03);
    return;
  }
}

void CPanel::ExtractArchives()
{
  if (_parentFolders.Size() > 0)
  {
    _panelCallback->OnCopy(false, false);
    return;
  }
  CRecordVector<UInt32> indices;
  GetOperatedItemIndices(indices);
  UStringVector paths;
  GetFilePaths(indices, paths);
  if (paths.IsEmpty())
    return;
  UString folderName;
  if (indices.Size() == 1)
    folderName = GetSubFolderNameForExtract(GetItemRelPath(indices[0]));
  else
    folderName = L"*";
  ::ExtractArchives(paths, _currentFolderPrefix + folderName + UString(WCHAR_PATH_SEPARATOR), true);
}

static void AddValuePair(UINT resourceID, UInt32 langID, UInt64 value, UString &s)
{
  wchar_t sz[32];
  s += LangString(resourceID, langID);
  s += L' ';
  ConvertUInt64ToString(value, sz);
  s += sz;
  s += L'\n';
}

class CThreadTest: public CProgressThreadVirt
{
  HRESULT ProcessVirt();
public:
  CRecordVector<UInt32> Indices;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderArchiveExtractCallback> ExtractCallback;
  CMyComPtr<IArchiveFolder> ArchiveFolder;
};

HRESULT CThreadTest::ProcessVirt()
{
  RINOK(ArchiveFolder->Extract(&Indices[0], Indices.Size(),
      NExtract::NPathMode::kFullPathnames, NExtract::NOverwriteMode::kAskBefore,
      NULL, BoolToInt(true), ExtractCallback));
  if (ExtractCallbackSpec->IsOK())
  {
    UString s;
    AddValuePair(IDS_FOLDERS_COLON, 0x02000321, ExtractCallbackSpec->NumFolders, s);
    AddValuePair(IDS_FILES_COLON, 0x02000320, ExtractCallbackSpec->NumFiles, s);
    // AddSizePair(IDS_SIZE_COLON, 0x02000322, Stat.UnpackSize, s);
    // AddSizePair(IDS_COMPRESSED_COLON, 0x02000323, Stat.PackSize, s);
    s += L'\n';
    s += LangString(IDS_MESSAGE_NO_ERRORS, 0x02000608);
    OkMessage = s;
  }
  return S_OK;
}

/*
static void AddSizePair(UINT resourceID, UInt32 langID, UInt64 value, UString &s)
{
  wchar_t sz[32];
  s += LangString(resourceID, langID);
  s += L" ";
  ConvertUInt64ToString(value, sz);
  s += sz;
  ConvertUInt64ToString(value >> 20, sz);
  s += L" (";
  s += sz;
  s += L" MB)";
  s += L'\n';
}
*/

void CPanel::TestArchives()
{
  CRecordVector<UInt32> indices;
  GetOperatedIndicesSmart(indices);
  CMyComPtr<IArchiveFolder> archiveFolder;
  _folder.QueryInterface(IID_IArchiveFolder, &archiveFolder);
  if (archiveFolder)
  {
    {
    CThreadTest extracter;

    extracter.ArchiveFolder = archiveFolder;
    extracter.ExtractCallbackSpec = new CExtractCallbackImp;
    extracter.ExtractCallback = extracter.ExtractCallbackSpec;
    extracter.ExtractCallbackSpec->ProgressDialog = &extracter.ProgressDialog;

    if (indices.IsEmpty())
      return;

    extracter.Indices = indices;
    
    UString title = LangString(IDS_PROGRESS_TESTING, 0x02000F90);
    UString progressWindowTitle = LangString(IDS_APP_TITLE, 0x03000000);
    
    extracter.ProgressDialog.CompressingMode = false;
    extracter.ProgressDialog.MainWindow = GetParent();
    extracter.ProgressDialog.MainTitle = progressWindowTitle;
    extracter.ProgressDialog.MainAddTitle = title + L" ";
    
    extracter.ExtractCallbackSpec->OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
    extracter.ExtractCallbackSpec->Init();
    
    if (extracter.Create(title, GetParent()) != S_OK)
      return;
    
    }
    RefreshTitleAlways();
    return;
  }

  if (!IsFSFolder())
  {
    MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
    return;
  }
  UStringVector paths;
  GetFilePaths(indices, paths);
  if (paths.IsEmpty())
    return;
  ::TestArchives(paths);
}
