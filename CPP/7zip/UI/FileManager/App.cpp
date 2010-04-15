// App.cpp

#include "StdAfx.h"

#include "resource.h"
#include "OverwriteDialogRes.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/Error.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/Thread.h"

#include "App.h"
#include "CopyDialog.h"
#include "ExtractCallback.h"
#include "FormatUtils.h"
#include "IFolder.h"
#include "LangUtils.h"
#include "RegistryUtils.h"
#include "ViewSettings.h"

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
  _app->RefreshTitle();
  _app->Panels[newPanelIndex]._headerComboBox.SetFocus();
  _app->Panels[newPanelIndex]._headerComboBox.ShowDropDown();
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
  
  if (ReadSingleClick())
  {
    extendedStyle |= LVS_EX_ONECLICKACTIVATE | LVS_EX_TRACKSELECT;
    /*
    if (ReadUnderline())
      extendedStyle |= LVS_EX_UNDERLINEHOT;
    */
  }

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

#ifndef ILC_COLOR32
#define ILC_COLOR32 0x0020
#endif

HRESULT CApp::CreateOnePanel(int panelIndex, const UString &mainPath, const UString &arcFormat,
  bool &archiveIsOpened, bool &encrypted)
{
  if (PanelsCreated[panelIndex])
    return S_OK;
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
  RINOK(Panels[panelIndex].Create(_window, _window,
      id, path, arcFormat, &m_PanelCallbackImp[panelIndex], &AppState, archiveIsOpened, encrypted));
  PanelsCreated[panelIndex] = true;
  return S_OK;
}

static void CreateToolbar(HWND parent,
    NWindows::NControl::CImageList &imageList,
    NWindows::NControl::CToolBar &toolBar,
    bool largeButtons)
{
  toolBar.Attach(::CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 0
      | WS_CHILD
      | WS_VISIBLE
      | TBSTYLE_FLAT
      | TBSTYLE_TOOLTIPS
      | TBSTYLE_WRAPABLE
      // | TBSTYLE_AUTOSIZE
      // | CCS_NORESIZE
      #ifdef UNDER_CE
      | CCS_NODIVIDER
      | CCS_NOPARENTALIGN
      #endif
      ,0,0,0,0, parent, NULL, g_hInstance, NULL));

  // TB_BUTTONSTRUCTSIZE message, which is required for
  // backward compatibility.
  toolBar.ButtonStructSize();

  imageList.Create(
      largeButtons ? 48: 24,
      largeButtons ? 36: 24,
      ILC_MASK | ILC_COLOR32, 0, 0);
  toolBar.SetImageList(0, imageList);
}

struct CButtonInfo
{
  int CommandID;
  UINT BitmapResID;
  UINT Bitmap2ResID;
  UINT StringResID;
  UInt32 LangID;
  UString GetText() const { return LangString(StringResID, LangID); }
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

static bool SetButtonText(int commandID, CButtonInfo *buttons, int numButtons, UString &s)
{
  for (int i = 0; i < numButtons; i++)
  {
    const CButtonInfo &b = buttons[i];
    if (b.CommandID == commandID)
    {
      s = b.GetText();
      return true;
    }
  }
  return false;
}

static void SetButtonText(int commandID, UString &s)
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
    CButtonInfo &butInfo, bool showText, bool large)
{
  TBBUTTON but;
  but.iBitmap = 0;
  but.idCommand = butInfo.CommandID;
  but.fsState = TBSTATE_ENABLED;
  but.fsStyle = TBSTYLE_BUTTON;
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

void CApp::ReloadToolbars()
{
  _buttonsImageList.Destroy();
  _toolBar.Destroy();


  if (ShowArchiveToolbar || ShowStandardToolbar)
  {
    CreateToolbar(_window, _buttonsImageList, _toolBar, LargeButtons);
    int i;
    if (ShowArchiveToolbar)
      for (i = 0; i < sizeof(g_ArchiveButtons) / sizeof(g_ArchiveButtons[0]); i++)
        AddButton(_buttonsImageList, _toolBar, g_ArchiveButtons[i], ShowButtonsLables, LargeButtons);
    if (ShowStandardToolbar)
      for (i = 0; i < sizeof(g_StandardButtons) / sizeof(g_StandardButtons[0]); i++)
        AddButton(_buttonsImageList, _toolBar, g_StandardButtons[i], ShowButtonsLables, LargeButtons);

    _toolBar.AutoSize();
  }
}

void CApp::SaveToolbarChanges()
{
  SaveToolbar();
  ReloadToolbars();
  MoveSubWindows();
}

void MyLoadMenu();

HRESULT CApp::Create(HWND hwnd, const UString &mainPath, const UString &arcFormat, int xSizes[2], bool &archiveIsOpened, bool &encrypted)
{
  _window.Attach(hwnd);
  #ifdef UNDER_CE
  _commandBar.Create(g_hInstance, hwnd, 1);
  #endif
  MyLoadMenu();
  #ifdef UNDER_CE
  _commandBar.AutoSize();
  #endif

  ReadToolbar();
  ReloadToolbars();

  int i;
  for (i = 0; i < kNumPanelsMax; i++)
    PanelsCreated[i] = false;

  AppState.Read();
  SetListSettings();
  SetShowSystemMenu();
  if (LastFocusedPanel >= kNumPanelsMax)
    LastFocusedPanel = 0;

  CListMode listMode;
  ReadListMode(listMode);
  for (i = 0; i < kNumPanelsMax; i++)
  {
    CPanel &panel = Panels[i];
    panel._ListViewMode = listMode.Panels[i];
    panel._xSize = xSizes[i];
    panel._flatModeForArc = ReadFlatView(i);
  }
  for (i = 0; i < kNumPanelsMax; i++)
    if (NumPanels > 1 || i == LastFocusedPanel)
    {
      if (NumPanels == 1)
        Panels[i]._xSize = xSizes[0] + xSizes[1];
      bool archiveIsOpened2 = false;
      bool encrypted2 = false;
      bool mainPanel = (i == LastFocusedPanel);
      RINOK(CreateOnePanel(i, mainPanel ? mainPath : L"", arcFormat, archiveIsOpened2, encrypted2));
      if (mainPanel)
      {
        archiveIsOpened = archiveIsOpened2;
        encrypted = encrypted2;
      }
    }
  SetFocusedPanel(LastFocusedPanel);
  Panels[LastFocusedPanel].SetFocusToList();
  return S_OK;
}

HRESULT CApp::SwitchOnOffOnePanel()
{
  if (NumPanels == 1)
  {
    NumPanels++;
    bool archiveIsOpened, encrypted;
    RINOK(CreateOnePanel(1 - LastFocusedPanel, UString(), UString(), archiveIsOpened, encrypted));
    Panels[1 - LastFocusedPanel].Enable(true);
    Panels[1 - LastFocusedPanel].Show(SW_SHOWNORMAL);
  }
  else
  {
    NumPanels--;
    Panels[1 - LastFocusedPanel].Enable(false);
    Panels[1 - LastFocusedPanel].Show(SW_HIDE);
  }
  MoveSubWindows();
  return S_OK;
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
    SaveFlatView(i, panel._flatModeForArc);
  }
  SaveListMode(listMode);
}

void CApp::Release()
{
  // It's for unloading COM dll's: don't change it.
  for (int i = 0; i < kNumPanelsMax; i++)
    Panels[i].Release();
}

// reduces path to part that exists on disk
static void ReducePathToRealFileSystemPath(UString &path)
{
  while (!path.IsEmpty())
  {
    if (NFind::DoesDirExist(path))
    {
      NName::NormalizeDirPathPrefix(path);
      break;
    }
    int pos = path.ReverseFind(WCHAR_PATH_SEPARATOR);
    if (pos < 0)
      path.Empty();
    else
    {
      path = path.Left(pos + 1);
      if (path.Length() == 3 && path[1] == L':')
        break;
      if (path.Length() > 2 && path[0] == '\\' && path[1] == '\\')
      {
        int nextPos = path.Find(WCHAR_PATH_SEPARATOR, 2); // pos after \\COMPNAME
        if (nextPos > 0 && path.Find(WCHAR_PATH_SEPARATOR, nextPos + 1) == pos)
          break;
      }
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
  if (path.Length() >= 1 && path[0] == WCHAR_PATH_SEPARATOR)
    return true;
  #ifdef _WIN32
  if (path.Length() >= 3 && path[1] == L':' && path[2] == L'\\')
    return true;
  #endif
  return false;
}

extern UString ConvertSizeToString(UInt64 value);

static UString AddSizeValue(UInt64 size)
{
  return MyFormatNew(IDS_FILE_SIZE, 0x02000982, ConvertSizeToString(size));
}

static void AddValuePair1(UINT resourceID, UInt32 langID, UInt64 size, UString &s)
{
  s += LangString(resourceID, langID);
  s += L" ";
  s += AddSizeValue(size);
  s += L"\n";
}

void AddValuePair2(UINT resourceID, UInt32 langID, UInt64 num, UInt64 size, UString &s)
{
  if (num == 0)
    return;
  s += LangString(resourceID, langID);
  s += L" ";
  s += ConvertSizeToString(num);

  if (size != (UInt64)(Int64)-1)
  {
    s += L"    ( ";
    s += AddSizeValue(size);
    s += L" )";
  }
  s += L"\n";
}

static void AddPropValueToSum(IFolderFolder *folder, int index, PROPID propID, UInt64 &sum)
{
  if (sum == (UInt64)(Int64)-1)
    return;
  NCOM::CPropVariant prop;
  folder->GetProperty(index, propID, &prop);
  switch(prop.vt)
  {
    case VT_UI4:
    case VT_UI8:
      sum += ConvertPropVariantToUInt64(prop);
      break;
    default:
      sum = (UInt64)(Int64)-1;
  }
}

UString CPanel::GetItemsInfoString(const CRecordVector<UInt32> &indices)
{
  UString info;
  UInt64 numDirs, numFiles, filesSize, foldersSize;
  numDirs = numFiles = filesSize = foldersSize = 0;
  int i;
  for (i = 0; i < indices.Size(); i++)
  {
    int index = indices[i];
    if (IsItemFolder(index))
    {
      AddPropValueToSum(_folder, index, kpidSize, foldersSize);
      numDirs++;
    }
    else
    {
      AddPropValueToSum(_folder, index, kpidSize, filesSize);
      numFiles++;
    }
  }

  AddValuePair2(IDS_FOLDERS_COLON, 0x02000321, numDirs, foldersSize, info);
  AddValuePair2(IDS_FILES_COLON, 0x02000320, numFiles, filesSize, info);
  int numDefined = ((foldersSize != (UInt64)(Int64)-1) && foldersSize != 0) ? 1: 0;
  numDefined += ((filesSize != (UInt64)(Int64)-1) && filesSize != 0) ? 1: 0;
  if (numDefined == 2)
    AddValuePair1(IDS_SIZE_COLON, 0x02000322, filesSize + foldersSize, info);
  
  info += L"\n";
  info += _currentFolderPrefix;
  
  for (i = 0; i < indices.Size() && i < kCopyDialog_NumInfoLines - 6; i++)
  {
    info += L"\n  ";
    int index = indices[i];
    info += GetItemRelPath(index);
    if (IsItemFolder(index))
      info += WCHAR_PATH_SEPARATOR;
  }
  if (i != indices.Size())
    info += L"\n  ...";
  return info;
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
    srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
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
      srcPanel.GetOperatedIndicesSmart(indices);
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

    copyDialog.Info = srcPanel.GetItemsInfoString(indices);

    if (copyDialog.Create(srcPanel.GetParent()) == IDCANCEL)
      return;

    destPath = copyDialog.Value;

    if (destPath.IsEmpty())
    {
      srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
      return;
    }

    if (!IsPathAbsolute(destPath))
    {
      if (!srcPanel.IsFSFolder())
      {
        srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
        return;
      }
      destPath = srcPanel._currentFolderPrefix + destPath;
    }

    #ifndef UNDER_CE
    if (destPath.Length() > 0 && destPath[0] == '\\')
      if (destPath.Length() == 1 || destPath[1] != '\\')
      {
        srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
        return;
      }
    #endif

    if (indices.Size() > 1 ||
        (!destPath.IsEmpty() && destPath.Back() == WCHAR_PATH_SEPARATOR) ||
        NFind::DoesDirExist(destPath) ||
        srcPanel.IsArcFolder())
    {
      NDirectory::CreateComplexDirectory(destPath);
      NName::NormalizeDirPathPrefix(destPath);
      if (!CheckFolderPath(destPath))
      {
        if (NumPanels < 2 || destPath != destPanel._currentFolderPrefix || !destPanel.DoesItSupportOperations())
        {
          srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
          return;
        }
        useDestPanel = true;
      }
    }
    else
    {
      int pos = destPath.ReverseFind(WCHAR_PATH_SEPARATOR);
      if (pos >= 0)
      {
        UString prefix = destPath.Left(pos + 1);
        NDirectory::CreateComplexDirectory(prefix);
        if (!CheckFolderPath(prefix))
        {
          srcPanel.MessageBoxErrorLang(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208);
          return;
        }
      }
    }

    AddUniqueStringToHeadOfList(copyFolders, destPath);
    while (copyFolders.Size() > 20)
      copyFolders.DeleteBack();
    SaveCopyHistory(copyFolders);
  }

  /*
  if (destPath == destPanel._currentFolderPrefix)
  {
    if (destPanel.GetFolderTypeID() == L"PhysDrive")
      useDestPanel = true;
  }
  */

  bool useSrcPanel = (!useDestPanel || !srcPanel.IsFsOrDrivesFolder() || destPanel.IsFSFolder());
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
    for (int i = 0; i < indices.Size(); i++)
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

  // destPanel.BindToFolder(srcPanel._currentFolderPrefix + srcPanel.GetItemName(realIndex) + WCHAR_PATH_SEPARATOR);

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
  {
    if (pnmh->code == TTN_GETDISPINFO)
    {
      LPNMTTDISPINFO info = (LPNMTTDISPINFO)pnmh;
      info->hinst = 0;
      g_ToolTipBuffer.Empty();
      SetButtonText((int)info->hdr.idFrom, g_ToolTipBuffer);
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
      SetButtonText((int)info->hdr.idFrom, g_ToolTipBuffer);
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
