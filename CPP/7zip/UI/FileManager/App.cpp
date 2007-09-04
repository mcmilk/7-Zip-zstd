// App.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/Error.h"
#include "Windows/COM.h"
#include "Windows/Thread.h"
#include "IFolder.h"

#include "App.h"

#include "CopyDialog.h"

#include "ExtractCallback.h"
#include "ViewSettings.h"
#include "RegistryUtils.h"
#include "LangUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

extern DWORD g_ComCtl32Version;
extern HINSTANCE g_hInstance;

static LPCWSTR kTempDirPrefix = L"7zE"; 

void CPanelCallbackImp::OnTab()
{
  if (g_App.NumPanels != 1)
    _app->Panels[1 - _index].SetFocusToList();  
  _app->RefreshTitle();
}

void CPanelCallbackImp::SetFocusToPath(int index)
{ 
  int newPanelIndex = index;
  if (g_App.NumPanels == 1)
    newPanelIndex = g_App.LastFocusedPanel;
  _app->Panels[newPanelIndex]._headerComboBox.SetFocus();
  _app->RefreshTitle();
}


void CPanelCallbackImp::OnCopy(bool move, bool copyToSame) { _app->OnCopy(move, copyToSame, _index); }
void CPanelCallbackImp::OnSetSameFolder() { _app->OnSetSameFolder(_index); }
void CPanelCallbackImp::OnSetSubFolder()  { _app->OnSetSubFolder(_index); }
void CPanelCallbackImp::PanelWasFocused() { _app->SetFocusedPanel(_index); _app->RefreshTitle(_index); }
void CPanelCallbackImp::DragBegin() { _app->DragBegin(_index); }
void CPanelCallbackImp::DragEnd() { _app->DragEnd(); }
void CPanelCallbackImp::RefreshTitle(bool always) { _app->RefreshTitle(_index, always); }

void CApp::SetListSettings()
{
  bool showDots = ReadShowDots();
  bool showRealFileIcons = ReadShowRealFileIcons();

  DWORD extendedStyle = LVS_EX_HEADERDRAGDROP;
  if (ReadFullRow())
    extendedStyle |= LVS_EX_FULLROWSELECT;
  if (ReadShowGrid())
    extendedStyle |= LVS_EX_GRIDLINES;
  bool mySelectionMode = ReadAlternativeSelection();
  
  /*
  if (ReadSingleClick())
  {
    extendedStyle |= LVS_EX_ONECLICKACTIVATE 
      | LVS_EX_TRACKSELECT;
    if (ReadUnderline())
      extendedStyle |= LVS_EX_UNDERLINEHOT;
  }
  */

  for (int i = 0; i < kNumPanelsMax; i++)
  {
    CPanel &panel = Panels[i];
    panel._mySelectMode = mySelectionMode;
    panel._showDots = showDots;
    panel._showRealFileIcons = showRealFileIcons;
    panel._exStyle = extendedStyle;

    DWORD style = (DWORD)panel._listView.GetStyle();
    if (mySelectionMode)
      style |= LVS_SINGLESEL;
    else
      style &= ~LVS_SINGLESEL;
    panel._listView.SetStyle(style);
    panel.SetExtendedStyle();
  }
}

void CApp::SetShowSystemMenu()
{
  ShowSystemMenu = ReadShowSystemMenu();
}

void CApp::CreateOnePanel(int panelIndex, const UString &mainPath, bool &archiveIsOpened, bool &encrypted)
{
  if (PanelsCreated[panelIndex])
    return;
  m_PanelCallbackImp[panelIndex].Init(this, panelIndex);
  UString path;
  if (mainPath.IsEmpty())
  {
    if (!::ReadPanelPath(panelIndex, path))
      path.Empty();
  }
  else
    path = mainPath;
  int id = 1000 + 100 * panelIndex;
  Panels[panelIndex].Create(_window, _window, 
      id, path, &m_PanelCallbackImp[panelIndex], &AppState, archiveIsOpened, encrypted);
  PanelsCreated[panelIndex] = true;
}

static void CreateToolbar(
    HWND parent,
    NWindows::NControl::CImageList &imageList,
    NWindows::NControl::CToolBar &toolBar,
    bool LargeButtons)
{
  toolBar.Attach(::CreateWindowEx(0, 
      TOOLBARCLASSNAME,
      NULL, 0
      | WS_VISIBLE
      | TBSTYLE_FLAT
      | TBSTYLE_TOOLTIPS 
      | WS_CHILD
      | CCS_NOPARENTALIGN
      | CCS_NORESIZE 
      | CCS_NODIVIDER
      // | TBSTYLE_AUTOSIZE
      // | CCS_ADJUSTABLE 
      ,0,0,0,0, parent, NULL, g_hInstance, NULL));

  // TB_BUTTONSTRUCTSIZE message, which is required for 
  // backward compatibility.
  toolBar.ButtonStructSize();

  imageList.Create(
      LargeButtons ? 48: 24, 
      LargeButtons ? 36: 24, 
      ILC_MASK, 0, 0);
  toolBar.SetImageList(0, imageList);
}

struct CButtonInfo
{
  UINT commandID;
  UINT BitmapResID;
  UINT Bitmap2ResID;
  UINT StringResID; 
  UINT32 LangID;
  UString GetText()const { return LangString(StringResID, LangID); };
};

static CButtonInfo g_StandardButtons[] = 
{
  { IDM_COPY_TO, IDB_COPY, IDB_COPY2, IDS_BUTTON_COPY, 0x03020420},
  { IDM_MOVE_TO, IDB_MOVE, IDB_MOVE2, IDS_BUTTON_MOVE, 0x03020421},
  { IDM_DELETE, IDB_DELETE, IDB_DELETE2, IDS_BUTTON_DELETE, 0x03020422} ,
  { IDM_FILE_PROPERTIES, IDB_INFO, IDB_INFO2, IDS_BUTTON_INFO, 0x03020423} 
};

static CButtonInfo g_ArchiveButtons[] = 
{
  { kAddCommand, IDB_ADD, IDB_ADD2, IDS_ADD, 0x03020400},
  { kExtractCommand, IDB_EXTRACT, IDB_EXTRACT2, IDS_EXTRACT, 0x03020401},
  { kTestCommand , IDB_TEST, IDB_TEST2, IDS_TEST, 0x03020402}
};

bool SetButtonText(UINT32 commandID, CButtonInfo *buttons, int numButtons, UString &s)
{
  for (int i = 0; i < numButtons; i++)
  {
    const CButtonInfo &b = buttons[i];
    if (b.commandID == commandID)
    {
      s = b.GetText();
      return true;
    }
  }
  return false;
}

void SetButtonText(UINT32 commandID, UString &s)
{
  if (SetButtonText(commandID, g_StandardButtons, 
      sizeof(g_StandardButtons) / sizeof(g_StandardButtons[0]), s))
    return;
  SetButtonText(commandID, g_ArchiveButtons, 
      sizeof(g_ArchiveButtons) / sizeof(g_ArchiveButtons[0]), s);
}

static void AddButton(
    NControl::CImageList &imageList,
    NControl::CToolBar &toolBar, 
    CButtonInfo &butInfo,
    bool showText,
    bool large)
{
  TBBUTTON but; 
  but.iBitmap = 0; 
  but.idCommand = butInfo.commandID; 
  but.fsState = TBSTATE_ENABLED; 
  but.fsStyle = BTNS_BUTTON
    // | BTNS_AUTOSIZE 
    ;
  but.dwData = 0;

  UString s = butInfo.GetText();
  but.iString = 0;
  if (showText)
    but.iString = (INT_PTR)(LPCWSTR)s; 

  but.iBitmap = imageList.GetImageCount();
  HBITMAP b = ::LoadBitmap(g_hInstance, 
      large ? 
      MAKEINTRESOURCE(butInfo.BitmapResID):
      MAKEINTRESOURCE(butInfo.Bitmap2ResID));
  if (b != 0)
  {
    imageList.AddMasked(b, RGB(255, 0, 255));
    ::DeleteObject(b);
  }
  #ifdef _UNICODE
  toolBar.AddButton(1, &but);
  #else
  toolBar.AddButtonW(1, &but);
  #endif
}

static void AddBand(NControl::CReBar &reBar, NControl::CToolBar &toolBar)
{
  SIZE size;
  toolBar.GetMaxSize(&size);

  RECT rect;
  toolBar.GetWindowRect(&rect);
  
  REBARBANDINFO rbBand;
  rbBand.cbSize = sizeof(REBARBANDINFO);  // Required
  rbBand.fMask  = RBBIM_STYLE 
    | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
  rbBand.fStyle = RBBS_CHILDEDGE; // RBBS_NOGRIPPER;
  rbBand.cxMinChild = size.cx; // rect.right - rect.left;
  rbBand.cyMinChild = size.cy; // rect.bottom - rect.top;
  rbBand.cyChild = rbBand.cyMinChild;
  rbBand.cx = rbBand.cxMinChild;
  rbBand.cxIdeal = rbBand.cxMinChild;
  rbBand.hwndChild = toolBar;
  reBar.InsertBand(-1, &rbBand);
}

void CApp::ReloadToolbars()
{ 
  if (!_rebar)
    return;
  HWND parent = _rebar;

  while(_rebar.GetBandCount() > 0)
    _rebar.DeleteBand(0);

  _archiveToolBar.Destroy();
  _archiveButtonsImageList.Destroy();

  _standardButtonsImageList.Destroy();
  _standardToolBar.Destroy();

  if (ShowArchiveToolbar)
  {
    CreateToolbar(parent, _archiveButtonsImageList, _archiveToolBar, LargeButtons);
    for (int i = 0; i < sizeof(g_ArchiveButtons) / sizeof(g_ArchiveButtons[0]); i++)
      AddButton(_archiveButtonsImageList, _archiveToolBar, g_ArchiveButtons[i], 
          ShowButtonsLables, LargeButtons);
    AddBand(_rebar, _archiveToolBar);
  }

  if (ShowStandardToolbar)
  {
    CreateToolbar(parent, _standardButtonsImageList, _standardToolBar, LargeButtons);
    for (int i = 0; i < sizeof(g_StandardButtons) / sizeof(g_StandardButtons[0]); i++)
      AddButton(_standardButtonsImageList, _standardToolBar, g_StandardButtons[i], 
          ShowButtonsLables, LargeButtons);
    AddBand(_rebar, _standardToolBar);
  }
}

void CApp::ReloadRebar(HWND hwnd)
{
  _rebar.Destroy();
  if (!ShowArchiveToolbar && !ShowStandardToolbar)
    return;
  if (g_ComCtl32Version >= MAKELONG(71, 4))
  {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    _rebar.Attach(::CreateWindowEx(WS_EX_TOOLWINDOW,
      REBARCLASSNAME,
      NULL, 
      WS_VISIBLE 
      | WS_BORDER 
      | WS_CHILD 
      | WS_CLIPCHILDREN 
      | WS_CLIPSIBLINGS 
      // | CCS_NODIVIDER  
      // | CCS_NOPARENTALIGN  // it's bead for moveing of two bands
      // | CCS_TOP
      | RBS_VARHEIGHT 
      | RBS_BANDBORDERS
      // | RBS_AUTOSIZE
      ,0,0,0,0, hwnd, NULL, g_hInstance, NULL));
  }
  if (_rebar == 0)
    return;
  REBARINFO rbi;
  rbi.cbSize = sizeof(REBARINFO);  // Required when using this struct.
  rbi.fMask = 0;
  rbi.himl = (HIMAGELIST)NULL;
  _rebar.SetBarInfo(&rbi);
  ReloadToolbars();
}

void CApp::Create(HWND hwnd, const UString &mainPath, int xSizes[2], bool &archiveIsOpened, bool &encrypted)
{
  ReadToolbar();
  ReloadRebar(hwnd);

  int i;
  for (i = 0; i < kNumPanelsMax; i++)
    PanelsCreated[i] = false;

  _window.Attach(hwnd);
  AppState.Read();
  SetListSettings();
  SetShowSystemMenu();
  if (LastFocusedPanel >= kNumPanelsMax)
    LastFocusedPanel = 0;

  CListMode listMode;
  ReadListMode(listMode);
  for (i = 0; i < kNumPanelsMax; i++)
  {
    Panels[i]._ListViewMode = listMode.Panels[i];
    Panels[i]._xSize = xSizes[i];
  }
  for (i = 0; i < kNumPanelsMax; i++)
    if (NumPanels > 1 || i == LastFocusedPanel)
    {
      if (NumPanels == 1)
        Panels[i]._xSize = xSizes[0] + xSizes[1];
      bool archiveIsOpened2 = false;
      bool encrypted2 = false;
      bool mainPanel = (i == LastFocusedPanel);
      CreateOnePanel(i, mainPanel ? mainPath : L"", archiveIsOpened2, encrypted2);
      if (mainPanel)
      {
        archiveIsOpened = archiveIsOpened2;
        encrypted = encrypted2;
      }
    }
  SetFocusedPanel(LastFocusedPanel);
  Panels[LastFocusedPanel].SetFocusToList();
}

extern void MoveSubWindows(HWND hWnd);

void CApp::SwitchOnOffOnePanel()
{
  if (NumPanels == 1)
  {
    NumPanels++;
    bool archiveIsOpened, encrypted;
    CreateOnePanel(1 - LastFocusedPanel, UString(), archiveIsOpened, encrypted);
    Panels[1 - LastFocusedPanel].Enable(true);
    Panels[1 - LastFocusedPanel].Show(SW_SHOWNORMAL);
  }
  else
  {
    NumPanels--;
    Panels[1 - LastFocusedPanel].Enable(false);
    Panels[1 - LastFocusedPanel].Show(SW_HIDE);
  }
  MoveSubWindows(_window);
}

void CApp::Save()
{
  AppState.Save();
  CListMode listMode;
  for (int i = 0; i < kNumPanelsMax; i++)
  {
    const CPanel &panel = Panels[i];
    UString path;
    if (panel._parentFolders.IsEmpty())
      path = panel._currentFolderPrefix;
    else
      path = GetFolderPath(panel._parentFolders[0].ParentFolder);
    SavePanelPath(i, path);
    listMode.Panels[i] = panel.GetListViewMode();
  }
  SaveListMode(listMode);
}

void CApp::Release()
{
  // It's for unloading COM dll's: don't change it. 
  for (int i = 0; i < kNumPanelsMax; i++)
    Panels[i].Release();
}

static bool IsThereFolderOfPath(const UString &path)
{
  CFileInfoW fileInfo;
  if (!FindFile(path, fileInfo))
    return false;
  return fileInfo.IsDirectory();
}

// reduces path to part that exists on disk
static void ReducePathToRealFileSystemPath(UString &path)
{
  while(!path.IsEmpty())
  {
    if (IsThereFolderOfPath(path))
    {
      NName::NormalizeDirPathPrefix(path);
      break;
    }
    int pos = path.ReverseFind('\\');
    if (pos < 0)
      path.Empty();
    else
    {
      path = path.Left(pos + 1);
      if (path.Length() == 3 && path[1] == L':')
        break;
      path = path.Left(pos);
    }
  }
}

// return true for dir\, if dir exist
static bool CheckFolderPath(const UString &path)
{
  UString pathReduced = path;
  ReducePathToRealFileSystemPath(pathReduced);
  return (pathReduced == path);
}

static bool IsPathAbsolute(const UString &path)
{
  if ((path.Length() >= 1 && path[0] == L'\\') ||
      (path.Length() >= 3 && path[1] == L':' && path[2] == L'\\'))
    return true;
  return false;
}

void CApp::OnCopy(bool move, bool copyToSame, int srcPanelIndex)
{
  int destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  CPanel &srcPanel = Panels[srcPanelIndex];
  CPanel &destPanel = Panels[destPanelIndex];

  CPanel::CDisableTimerProcessing disableTimerProcessing1(destPanel);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(srcPanel);

  if (!srcPanel.DoesItSupportOperations())
  {
    srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
    return;
  }

  CRecordVector<UInt32> indices;
  UString destPath;
  bool useDestPanel = false;

  {
    if (copyToSame)
    {
      int focusedItem = srcPanel._listView.GetFocusedItem();
      if (focusedItem < 0)
        return;
      int realIndex = srcPanel.GetRealItemIndex(focusedItem);
      if (realIndex == kParentIndex)
        return;
      indices.Add(realIndex);
      destPath = srcPanel.GetItemName(realIndex);
    }
    else
    {
      srcPanel.GetOperatedItemIndices(indices);
      if (indices.Size() == 0)
        return;
      destPath = destPanel._currentFolderPrefix;
      if (NumPanels == 1)
        ReducePathToRealFileSystemPath(destPath);
    }

    CCopyDialog copyDialog;
    UStringVector copyFolders;
    ReadCopyHistory(copyFolders);

    copyDialog.Strings = copyFolders;
    copyDialog.Value = destPath;
    
    copyDialog.Title = move ? 
        LangString(IDS_MOVE, 0x03020202):
        LangString(IDS_COPY, 0x03020201);
    copyDialog.Static = move ? 
        LangString(IDS_MOVE_TO, 0x03020204):
        LangString(IDS_COPY_TO, 0x03020203);

    if (copyDialog.Create(srcPanel.GetParent()) == IDCANCEL)
      return;

    destPath = copyDialog.Value;

    if (!IsPathAbsolute(destPath))
    {
      if (!srcPanel.IsFSFolder())
      {
        srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
        return;
      }
      destPath = srcPanel._currentFolderPrefix + destPath;
    }

    if (indices.Size() > 1 || (destPath.Length() > 0 && destPath.ReverseFind('\\') == destPath.Length() - 1) || 
        IsThereFolderOfPath(destPath))
    {
      NDirectory::CreateComplexDirectory(destPath);
      NName::NormalizeDirPathPrefix(destPath);
      if (!CheckFolderPath(destPath))
      {
        if (NumPanels < 2 || destPath != destPanel._currentFolderPrefix || !destPanel.DoesItSupportOperations())
        {
          srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
          return;
        }
        useDestPanel = true;
      }
    }
    else
    {
      int pos = destPath.ReverseFind('\\');
      if (pos >= 0)
      {
        UString prefix = destPath.Left(pos + 1);
        NDirectory::CreateComplexDirectory(prefix);
        if (!CheckFolderPath(prefix))
        {
          srcPanel.MessageBox(LangString(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
          return;
        }
      }
    }

    AddUniqueStringToHeadOfList(copyFolders, destPath);
    while (copyFolders.Size() > 20)
      copyFolders.DeleteBack();
    SaveCopyHistory(copyFolders);
  }

  bool useSrcPanel = (!useDestPanel || !srcPanel.IsFSFolder() || destPanel.IsFSFolder());
  bool useTemp = useSrcPanel && useDestPanel;
  NFile::NDirectory::CTempDirectoryW tempDirectory;
  UString tempDirPrefix;
  if (useTemp)
  {
    tempDirectory.Create(kTempDirPrefix);
    tempDirPrefix = tempDirectory.GetPath();
    NFile::NName::NormalizeDirPathPrefix(tempDirPrefix);
  }

  CSelectedState srcSelState;
  CSelectedState destSelState;
  srcPanel.SaveSelectedState(srcSelState);
  destPanel.SaveSelectedState(destSelState);

  HRESULT result;
  if (useSrcPanel)
  {
    UString folder = useTemp ? tempDirPrefix : destPath;
    result = srcPanel.CopyTo(indices, folder, move, true, 0);
    if (result != S_OK)
    {
      disableTimerProcessing1.Restore();
      disableTimerProcessing2.Restore();
      // For Password:
      srcPanel.SetFocusToList();
      if (result != E_ABORT)
        srcPanel.MessageBoxError(result, L"Error");
      return;
    }
  }
  
  if (useDestPanel)
  {
    UStringVector filePaths;
    UString folderPrefix;
    if (useTemp)
      folderPrefix = tempDirPrefix;
    else
      folderPrefix = srcPanel._currentFolderPrefix;
    filePaths.Reserve(indices.Size());
    for(int i = 0; i < indices.Size(); i++)
      filePaths.Add(srcPanel.GetItemRelPath(indices[i]));

    result = destPanel.CopyFrom(folderPrefix, filePaths, true, 0);

    if (result != S_OK)
    {
      disableTimerProcessing1.Restore();
      disableTimerProcessing2.Restore();
      // For Password:
      srcPanel.SetFocusToList();
      if (result != E_ABORT)
        srcPanel.MessageBoxError(result, L"Error");
      return;
    }
  }

  RefreshTitleAlways();
  if (copyToSame || move)
  {
    srcPanel.RefreshListCtrl(srcSelState);
  }
  if (!copyToSame)
  {
    destPanel.RefreshListCtrl(destSelState);
    srcPanel.KillSelection();
  }
  disableTimerProcessing1.Restore();
  disableTimerProcessing2.Restore();
  srcPanel.SetFocusToList();
}

void CApp::OnSetSameFolder(int srcPanelIndex)
{
  if (NumPanels <= 1)
    return;
  const CPanel &srcPanel = Panels[srcPanelIndex];
  CPanel &destPanel = Panels[1 - srcPanelIndex];
  destPanel.BindToPathAndRefresh(srcPanel._currentFolderPrefix);
}

void CApp::OnSetSubFolder(int srcPanelIndex)
{
  if (NumPanels <= 1)
    return;
  const CPanel &srcPanel = Panels[srcPanelIndex];
  CPanel &destPanel = Panels[1 - srcPanelIndex];

  int focusedItem = srcPanel._listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  int realIndex = srcPanel.GetRealItemIndex(focusedItem);
  if (!srcPanel.IsItemFolder(realIndex))
    return;


  /*
  UString string = srcPanel._currentFolderPrefix + 
      srcPanel.GetItemName(realIndex) + L'\\';
  destPanel.BindToFolder(string);
  */
  CMyComPtr<IFolderFolder> newFolder;
  if (realIndex == kParentIndex)
  {
    if (srcPanel._folder->BindToParentFolder(&newFolder) != S_OK)
      return;
  }
  else
  {
    if (srcPanel._folder->BindToFolder(realIndex, &newFolder) != S_OK)
      return;
  }
  destPanel.CloseOpenFolders();
  destPanel._folder = newFolder;
  destPanel.RefreshListCtrl();
}

/*
int CApp::GetFocusedPanelIndex() const
{
  return LastFocusedPanel;
  HWND hwnd = ::GetFocus();
  for (;;)
  {
    if (hwnd == 0)
      return 0;
    for (int i = 0; i < kNumPanelsMax; i++)
    {
      if (PanelsCreated[i] && 
          ((HWND)Panels[i] == hwnd || Panels[i]._listView == hwnd))
        return i;
    }
    hwnd = GetParent(hwnd);
  }
}
  */

static UString g_ToolTipBuffer;
static CSysString g_ToolTipBufferSys;

void CApp::OnNotify(int /* ctrlID */, LPNMHDR pnmh)
{
  if (pnmh->hwndFrom == _rebar)
  {
    switch(pnmh->code)
    {
      case RBN_HEIGHTCHANGE:
      {
        MoveSubWindows(g_HWND);
        return;
      }
    }
    return ;
  }
  else 
  {
    if (pnmh->code == TTN_GETDISPINFO)
    {
      LPNMTTDISPINFO info = (LPNMTTDISPINFO)pnmh;
      info->hinst = 0;
      g_ToolTipBuffer.Empty();
      SetButtonText((UINT32)info->hdr.idFrom, g_ToolTipBuffer);
      g_ToolTipBufferSys = GetSystemString(g_ToolTipBuffer);
      info->lpszText = (LPTSTR)(LPCTSTR)g_ToolTipBufferSys;
      return;
    }
    #ifndef _UNICODE
    if (pnmh->code == TTN_GETDISPINFOW)
    {
      LPNMTTDISPINFOW info = (LPNMTTDISPINFOW)pnmh;
      info->hinst = 0;
      g_ToolTipBuffer.Empty();
      SetButtonText((UINT32)info->hdr.idFrom, g_ToolTipBuffer);
      info->lpszText = (LPWSTR)(LPCWSTR)g_ToolTipBuffer;
      return;
    }
    #endif
  }
}

void CApp::RefreshTitle(bool always)
{ 
  UString path = GetFocusedPanel()._currentFolderPrefix;
  if (path.IsEmpty())
    path += LangString(IDS_APP_TITLE, 0x03000000);
  if (!always && path == PrevTitle)
    return;
  PrevTitle = path;
  NWindows::MySetWindowText(_window, path);
}

void CApp::RefreshTitle(int panelIndex, bool always)
{ 
  if (panelIndex != GetFocusedPanelIndex())
    return;
  RefreshTitle(always);
}

