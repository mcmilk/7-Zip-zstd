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

#include "Resource/CopyDialog/CopyDialog.h"

#include "ExtractCallback.h"
#include "UpdateCallback100.h"
#include "ViewSettings.h"
#include "RegistryUtils.h"

using namespace NWindows;
using namespace NFile;
using namespace NFind;

void CPanelCallbackImp::OnTab()
{
  if (g_App.NumPanels != 1)
    _app->Panels[1 - _index].SetFocusToList();  
}

void CPanelCallbackImp::SetFocusToPath(int index)
{ 
  int newPanelIndex = index;
  if (g_App.NumPanels == 1)
    newPanelIndex = g_App.LastFocusedPanel;
  _app->Panels[newPanelIndex]._headerComboBox.SetFocus();
}


void CPanelCallbackImp::OnCopy(bool move, bool copyToSame)
  { _app->OnCopy(move, copyToSame, _index); }

void CPanelCallbackImp::OnSetSameFolder()
  { _app->OnSetSameFolder(_index); }

void CPanelCallbackImp::OnSetSubFolder()
  { _app->OnSetSubFolder(_index); }

void CPanelCallbackImp::PanelWasFocused()
{ 
  _app->LastFocusedPanel = _index; 
}

void CApp::SetListSettings()
{
  bool showDots = ReadShowDots();
  bool showRealFileIcons = ReadShowRealFileIcons();
  for (int i = 0; i < kNumPanelsMax; i++)
  {
    Panels[i]._showDots = showDots;
    Panels[i]._showRealFileIcons = showRealFileIcons;
  }
}

void CApp::SetShowSystemMenu()
{
  ShowSystemMenu = ReadShowSystemMenu();
}

void CApp::CreateOnePanel(int panelIndex, const UString &mainPath)
{
  if (PanelsCreated[panelIndex])
    return;
  m_PanelCallbackImp[panelIndex].Init(this, panelIndex);
  UString path;
  if (mainPath.IsEmpty())
  {
    CSysString sysPath; 
    if (!::ReadPanelPath(panelIndex, sysPath))
      sysPath.Empty();
    path = GetUnicodeString(sysPath);
  }
  else
    path = mainPath;
  int id = 1000 + 100 * panelIndex;
  Panels[panelIndex].Create(_window, _window, 
      id, 0, path, &m_PanelCallbackImp[panelIndex], &_appState);
  PanelsCreated[panelIndex] = true;
}

void CApp::Create(HWND hwnd, const UString &mainPath)
{
  int i;
  for (i = 0; i < kNumPanelsMax; i++)
    PanelsCreated[i] = false;

  _window.Attach(hwnd);
  _appState.Read();
  SetListSettings();
  SetShowSystemMenu();
  UString mainPathSpec = mainPath;
  if (LastFocusedPanel >= kNumPanelsMax)
    LastFocusedPanel = 0;
  for (i = 0; i < kNumPanelsMax; i++)
    if (NumPanels > 1 || i == LastFocusedPanel)
      CreateOnePanel(i, (i == LastFocusedPanel) ? mainPath : L"");
  Panels[LastFocusedPanel].SetFocusToList();
}

extern void MoveSubWindows(HWND hWnd);

void CApp::SwitchOnOffOnePanel()
{
  if (NumPanels == 1)
  {
    NumPanels++;
    CreateOnePanel(1 - LastFocusedPanel, UString());
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
  _appState.Save();
  for (int i = 0; i < kNumPanelsMax; i++)
  {
    const CPanel &panel = Panels[i];
    UString path;
    if (panel._parentFolders.IsEmpty())
      path = panel._currentFolderPrefix;
    else
      path = GetFolderPath(panel._parentFolders[0].ParentFolder);
    SavePanelPath(i, GetSystemString(path));
  }
}

void CApp::Release()
{
  // It's for unloading COM dll's: don't change it. 
  for (int i = 0; i < kNumPanelsMax; i++)
    Panels[i].Release();
}

class CWindowDisable
{
  bool _wasEnabled;
  CWindow _window;
public:
  CWindowDisable(HWND window): _window(window) 
  { 
    _wasEnabled = _window.IsEnabled();
    if (_wasEnabled)
      _window.Enable(false); 
  }
  ~CWindowDisable() 
  { 
    if (_wasEnabled)
      _window.Enable(true); 
  }
};

struct CThreadExtract
{
  bool Move;
  CMyComPtr<IFolderOperations> FolderOperations;
  CRecordVector<UINT32> Indices;
  UString DestPath;
  CExtractCallbackImp *ExtractCallbackSpec;
  CMyComPtr<IFolderOperationsExtractCallback> ExtractCallback;
  HRESULT Result;
  
  DWORD Extract()
  {
    NCOM::CComInitializer comInitializer;
    ExtractCallbackSpec->ProgressDialog.WaitCreating();
    if (Move)
    {
      Result = FolderOperations->MoveTo(
          &Indices.Front(), Indices.Size(), 
          DestPath, ExtractCallback);
      // ExtractCallbackSpec->DestroyWindows();
    }
    else
    {
      Result = FolderOperations->CopyTo(
          &Indices.Front(), Indices.Size(), 
          DestPath, ExtractCallback);
      // ExtractCallbackSpec->DestroyWindows();
    }
    ExtractCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadExtract *)param)->Extract();
  }
};

struct CThreadUpdate
{
  bool Move;
  CMyComPtr<IFolderOperations> FolderOperations;
  UString SrcFolderPrefix;
  UStringVector FileNames;
  CRecordVector<const wchar_t *> FileNamePointers;
  CMyComPtr<IFolderArchiveUpdateCallback> UpdateCallback;
  CUpdateCallback100Imp *UpdateCallbackSpec;
  HRESULT Result;
  
  DWORD Process()
  {
    NCOM::CComInitializer comInitializer;
    UpdateCallbackSpec->ProgressDialog.WaitCreating();
    if (Move)
    {
      {
        throw 1;
        // srcPanel.MessageBoxMyError(L"Move is not supported");
        return 0;
      }
    }
    else
    {
      Result = FolderOperations->CopyFrom(
          SrcFolderPrefix,
          &FileNamePointers.Front(),
          FileNamePointers.Size(),
          UpdateCallback);
    }
    UpdateCallbackSpec->ProgressDialog.MyClose();
    return 0;
  }

  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadUpdate *)param)->Process();
  }
};



void CApp::OnCopy(bool move, bool copyToSame, int srcPanelIndex)
{
  int destPanelIndex = (NumPanels <= 1) ? srcPanelIndex : (1 - srcPanelIndex);
  CPanel &srcPanel = Panels[srcPanelIndex];
  CPanel &destPanel = Panels[destPanelIndex];
  bool useSrcPanel = true;
  if (NumPanels != 1)
  {
    if (!srcPanel.IsFSFolder() && !destPanel.IsFSFolder())
    {
      srcPanel.MessageBox(LangLoadStringW(IDS_CANNOT_COPY, 0x03020207));
      return;
    }
    useSrcPanel = copyToSame || destPanel.IsFSFolder();
    if (move && !useSrcPanel)
    {
      srcPanel.MessageBoxMyError(L"Move is not supported");
      return;
    }
  }

  CPanel &panel = useSrcPanel ? srcPanel : destPanel;

  CMyComPtr<IFolderOperations> folderOperations;

  // if (move)
    if (panel._folder.QueryInterface(IID_IFolderOperations, 
        &folderOperations) != S_OK)
    {
      panel.MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
      return;
    }

  CRecordVector<UINT32> indices;

  CCopyDialog copyDialog;

  UStringVector copyFolders;
  ReadCopyHistory(copyFolders);

  int i;
  for (i = 0; i < copyFolders.Size(); i++)
    copyDialog.Strings.Add(GetSystemString(copyFolders[i]));

  if (copyToSame)
  {
    int focusedItem = srcPanel._listView.GetFocusedItem();
    if (focusedItem < 0)
      return;
    int realIndex = srcPanel.GetRealItemIndex(focusedItem);
    if (realIndex == -1)
      return;
    indices.Add(realIndex);
    copyDialog.Value = GetSystemString(srcPanel.GetItemName(realIndex));
  }
  else
  {
    srcPanel.GetOperatedItemIndices(indices);
    if (indices.Size() == 0)
      return;
    CSysString destPath = GetSystemString(destPanel._currentFolderPrefix);
    if (NumPanels == 1)
    {
      while(!destPath.IsEmpty())
      {
        CFileInfo fileInfo;
        if (FindFile(destPath, fileInfo))
        {
          if (fileInfo.IsDirectory())
          {
            destPath += TEXT('\\');
            break;
          }
        }
        int pos = destPath.ReverseFind('\\');
        if (pos < 0)
          destPath.Empty();
        else
          destPath = destPath.Left(pos);
      }
    }
    copyDialog.Value = destPath;
  }
  copyDialog.Title = move ? 
    LangLoadString(IDS_MOVE, 0x03020202):
    LangLoadString(IDS_COPY, 0x03020201);
  copyDialog.Static = move ? 
    LangLoadString(IDS_MOVE_TO, 0x03020204):
    LangLoadString(IDS_COPY_TO, 0x03020203);
  if (copyDialog.Create(srcPanel.GetParent()) == IDCANCEL)
    return;

  AddUniqueStringToHeadOfList(copyFolders, GetUnicodeString(
      copyDialog.Value));
  while (copyFolders.Size() > 20)
    copyFolders.DeleteBack();

  SaveCopyHistory(copyFolders);
  
  /// ?????
  SetCurrentDirectory(GetSystemString(srcPanel._currentFolderPrefix));

  CSysString destPath;
  if (!NDirectory::MyGetFullPathName(copyDialog.Value, destPath))
  {
    srcPanel.MessageBoxLastError();
    return;
  }

  if (destPath.Length() > 0 && destPath.ReverseFind('\\') == destPath.Length() - 1)
    NDirectory::CreateComplexDirectory(destPath);

  CSysString title = move ? 
      LangLoadString(IDS_MOVING, 0x03020206):
      LangLoadString(IDS_COPYING, 0x03020205);
  CSysString progressWindowTitle = LangLoadString(IDS_APP_TITLE, 0x03000000);

  CPanel::CDisableTimerProcessing disableTimerProcessing1(destPanel);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(srcPanel);

  HRESULT result;
  if (useSrcPanel)
  {
    CThreadExtract extracter;
    extracter.ExtractCallbackSpec = new CExtractCallbackImp;
    extracter.ExtractCallback = extracter.ExtractCallbackSpec;
    extracter.ExtractCallbackSpec->_parentWindow = _window;

    extracter.ExtractCallbackSpec->ProgressDialog.MainWindow = _window;
    extracter.ExtractCallbackSpec->ProgressDialog.MainTitle = progressWindowTitle;
    extracter.ExtractCallbackSpec->ProgressDialog.MainAddTitle = title + CSysString(TEXT(" "));

    extracter.ExtractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, false, L"");
    extracter.Move = move;
    extracter.FolderOperations = folderOperations;
    extracter.Indices = indices;;
    extracter.DestPath = GetUnicodeString(destPath);
    CThread thread;
    if (!thread.Create(CThreadExtract::MyThreadFunction, &extracter))
      throw 271824;
    extracter.ExtractCallbackSpec->StartProgressDialog(title);
    result = extracter.Result;
  }
  else
  {
    CThreadUpdate updater;
    updater.UpdateCallbackSpec = new CUpdateCallback100Imp;
    updater.UpdateCallback = updater.UpdateCallbackSpec;
    
    updater.UpdateCallbackSpec->ProgressDialog.MainWindow = _window;
    updater.UpdateCallbackSpec->ProgressDialog.MainTitle = progressWindowTitle;
    updater.UpdateCallbackSpec->ProgressDialog.MainAddTitle = title + CSysString(TEXT(" "));

    updater.UpdateCallbackSpec->Init(_window, false, L"");
    updater.Move = move;
    updater.FolderOperations = folderOperations;
    updater.SrcFolderPrefix = srcPanel._currentFolderPrefix;
    updater.FileNames.Reserve(indices.Size());
    for(int i = 0; i < indices.Size(); i++)
      updater.FileNames.Add(srcPanel.GetItemName(indices[i]));
    updater.FileNamePointers.Reserve(indices.Size());
    for(i = 0; i < indices.Size(); i++)
      updater.FileNamePointers.Add(updater.FileNames[i]);
    CThread thread;
    if (!thread.Create(CThreadUpdate::MyThreadFunction, &updater))
      throw 271824;
    updater.UpdateCallbackSpec->StartProgressDialog(title);
    result = updater.Result;
  }

  /*
  if (useSrcPanel)
    extractCallbackSpec->DestroyWindows();
  */
  
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
  if (copyToSame || move)
  {
    srcPanel.RefreshListCtrlSaveFocused();
  }
  if (!copyToSame)
  {
    destPanel.RefreshListCtrlSaveFocused();
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
  if (realIndex == -1)
  {
    if (srcPanel._folder->BindToParentFolder(&newFolder) != S_OK)
      return;
  }
  else
  {
    if (srcPanel._folder->BindToFolder(realIndex, &newFolder) != S_OK)
      return;
  }
  destPanel._folder = newFolder;
  destPanel.RefreshListCtrl();
}

int CApp::GetFocusedPanelIndex()
{
  return LastFocusedPanel;
  /*
  HWND hwnd = ::GetFocus();
  while(true)
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
  */
}
