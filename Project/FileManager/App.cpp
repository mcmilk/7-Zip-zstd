// App.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/StringConvert.h"
#include "Windows/FileDir.h"
#include "Windows/Error.h"
#include "FolderInterface.h"

#include "App.h"

#include "Resource/ComboDialog/ComboDialog.h"

#include "ExtractCallback.h"
#include "UpdateCallback100.h"

using namespace NWindows;
using namespace NFile;

void CPanelCallbackImp::OnTab()
{
  if (kNumPanels != 1)
    _app->_panel[1 - _index].SetFocus();  
}

void CPanelCallbackImp::OnSetFocusToPath(int index)
{ 
  int newPanelIndex = index;
  if (newPanelIndex > kNumPanels)
    newPanelIndex = 0;
  _app->_panel[newPanelIndex].SetFocus();  
  _app->_panel[newPanelIndex]._headerComboBox.SetFocus();
}


void CPanelCallbackImp::OnCopy(bool move, bool copyToSame)
  { _app->OnCopy(move, copyToSame, _index); }

void CPanelCallbackImp::OnSetSameFolder()
  { _app->OnSetSameFolder(_index); }

void CPanelCallbackImp::OnSetSubFolder()
  { _app->OnSetSubFolder(_index); }

void CApp::Create(HWND hwnd, UString &mainPath)
{
  _window.Attach(hwnd);
  _appState.Read();
  for (int i = 0; i < kNumPanels; i++)
  {
    m_PanelCallbackImp[i].Init(this, i);
    CSysString path;
    if (i == 0 && !mainPath.IsEmpty())
    {
      path = GetSystemString(mainPath);
    }
    else
      if (!ReadPanelPath(i, path))
        path.Empty();
    int id = 1000 + 100 * i;
    _panel[i].Create(hwnd, hwnd, i, id, 0, path, 
      &m_PanelCallbackImp[i], &_appState);
  }
  _panel[0].SetFocus();
}

void CApp::Save()
{
  _appState.Save();
  for (int i = 0; i < kNumPanels; i++)
  {
    const CPanel &panel = _panel[i];
    UString path;
    if (panel._parentFolders.IsEmpty())
      path = panel._currentFolderPrefix;
    else
      path = GetFolderPath(panel._parentFolders[0].ParentFolder);
    SavePanelPath(i, GetSystemString(path));
  }
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

void CApp::OnCopy(bool move, bool copyToSame, int srcPanelIndex)
{
  if (kNumPanels <= 1)
    return;
  CPanel &srcPanel = _panel[srcPanelIndex];
  CPanel &destPanel = _panel[1 - srcPanelIndex];
  if (!srcPanel.IsFSFolder() && !destPanel.IsFSFolder())
  {
    srcPanel.MessageBox(LangLoadStringW(IDS_CANNOT_COPY, 0x03020207));
    return;
  }

  bool useSrcPanel = copyToSame || destPanel.IsFSFolder();
  
  CPanel &panel = useSrcPanel ? srcPanel : destPanel;

  CComPtr<IFolderOperations> folderOperations;

  // if (move)
    if (panel._folder.QueryInterface(&folderOperations) != S_OK)
    {
      panel.MessageBox(LangLoadStringW(IDS_OPERATION_IS_NOT_SUPPORTED, 0x03020208));
      return;
    }

  CRecordVector<UINT32> indices;

  CComboDialog comboDialog;

  if (copyToSame)
  {
    int focusedItem = srcPanel._listView.GetFocusedItem();
    if (focusedItem < 0)
      return;
    UINT32 realIndex = srcPanel.GetRealItemIndex(focusedItem);
    indices.Add(realIndex);
    comboDialog.Value = GetSystemString(srcPanel.GetItemName(realIndex));
  }
  else
  {
    srcPanel.GetOperatedItemIndexes(indices);
    comboDialog.Value = GetSystemString(destPanel._currentFolderPrefix);
  }
  comboDialog.Title = move ? 
    LangLoadString(IDS_MOVE, 0x03020202):
    LangLoadString(IDS_COPY, 0x03020201);
  comboDialog.Static = move ? 
    LangLoadString(IDS_MOVE_TO, 0x03020204):
    LangLoadString(IDS_COPY_TO, 0x03020203);
  if (comboDialog.Create(srcPanel.GetParent()) == IDCANCEL)
    return;
  
  /// ?????
  SetCurrentDirectory(GetSystemString(srcPanel._currentFolderPrefix));

  CSysString destPath;
  if (!NDirectory::MyGetFullPathName(comboDialog.Value, destPath))
  {
    srcPanel.MessageBoxLastError();
    return;
  }

  CComObjectNoLock<CExtractCallbackImp> *extractCallbackSpec;
  CComPtr<IFolderOperationsExtractCallback> extractCallback;
  CComObjectNoLock<CUpdateCallBack100Imp> *updateCallbackSpec;
  CComPtr<IUpdateCallback100> updateCallback;
  CSysString title = move ? 
      LangLoadString(IDS_MOVING, 0x03020206):
      LangLoadString(IDS_COPYING, 0x03020205);
  CSysString progressWindowTitle = LangLoadString(IDS_APP_TITLE, 0x03000000);
  if (useSrcPanel)
  {
    extractCallbackSpec = new CComObjectNoLock<CExtractCallbackImp>;
    extractCallback = extractCallbackSpec;
    extractCallbackSpec->_parentWindow = _window;
    extractCallbackSpec->_appTitle.Window = _window;
    extractCallbackSpec->_appTitle.Title = progressWindowTitle;
    extractCallbackSpec->_appTitle.AddTitle = title + CSysString(TEXT(" "));

    extractCallbackSpec->StartProgressDialog(title);
    extractCallbackSpec->Init(NExtractionMode::NOverwrite::kAskBefore, false, L"");
  }
  else
  {
    updateCallbackSpec = new CComObjectNoLock<CUpdateCallBack100Imp>;
    updateCallback = updateCallbackSpec;
    updateCallbackSpec->Init(_window, title, false, L"");
    updateCallbackSpec->_appTitle.Window = _window;
    updateCallbackSpec->_appTitle.Title = progressWindowTitle;
    updateCallbackSpec->_appTitle.AddTitle = title + CSysString(TEXT(" "));
  }
  
  CPanel::CDisableTimerProcessing disableTimerProcessing1(destPanel);
  CPanel::CDisableTimerProcessing disableTimerProcessing2(srcPanel);


  UStringVector fileNames;
  CRecordVector<const wchar_t *> fileNamePointers;
  if (!useSrcPanel)
  {
    fileNames.Reserve(indices.Size());
    for(int i = 0; i < indices.Size(); i++)
      fileNames.Add(srcPanel.GetItemName(indices[i]));
    fileNamePointers.Reserve(indices.Size());
    for(i = 0; i < indices.Size(); i++)
      fileNamePointers.Add(fileNames[i]);
  }

  HRESULT result;
  {
    CWindowDisable windowDisable(_window);
    if (move)
    {
      if (useSrcPanel)
        result = folderOperations->MoveTo(&indices.Front(), indices.Size(), 
          GetUnicodeString(destPath),
          extractCallback);
      else
      {
        srcPanel.MessageBoxMyError(L"Move is not supported");
        return;
      }
    }
    else
    {
      if (useSrcPanel)
        result = folderOperations->CopyTo(&indices.Front(), indices.Size(), 
            GetUnicodeString(destPath),
            extractCallback);
      else
      {
        result = folderOperations->CopyFrom(srcPanel._currentFolderPrefix,
            &fileNamePointers.Front(),
            fileNamePointers.Size(),
            updateCallback);
      }

      /*
      if (srcPanel._folder->Extract(&indices.Front(), indices.Size(), 
      NExtractionMode::NPath::kCurrentPathnames, 
      NExtractionMode::NOverwrite::kAskBefore, 
      GetUnicodeString(destPath),
      BoolToMyBool(false),
      extractCallback) != S_OK)
      */
    }
  }
  if (useSrcPanel)
    extractCallbackSpec->DestroyWindows();
  

  if (result != S_OK)
  {
    disableTimerProcessing1.Restore();
    disableTimerProcessing2.Restore();
    srcPanel.SetFocus();
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
  srcPanel.SetFocus();
}


void CApp::OnSetSameFolder(int srcPanelIndex)
{
  if (kNumPanels <= 1)
    return;
  const CPanel &srcPanel = _panel[srcPanelIndex];
  CPanel &destPanel = _panel[1 - srcPanelIndex];
  destPanel.BindToFolder(srcPanel._currentFolderPrefix);
}

void CApp::OnSetSubFolder(int srcPanelIndex)
{
  if (kNumPanels <= 1)
    return;
  const CPanel &srcPanel = _panel[srcPanelIndex];
  CPanel &destPanel = _panel[1 - srcPanelIndex];

  int focusedItem = srcPanel._listView.GetFocusedItem();
  if (focusedItem < 0)
    return;
  UINT32 realIndex = srcPanel.GetRealItemIndex(focusedItem);
  if (!srcPanel.IsItemFolder(realIndex))
    return;

  /*
  UString string = srcPanel._currentFolderPrefix + 
      srcPanel.GetItemName(realIndex) + L'\\';
  destPanel.BindToFolder(string);
  */
  CComPtr<IFolderFolder> newFolder;
  if (srcPanel._folder->BindToFolder(realIndex, &newFolder) != S_OK)
    return;
  destPanel._folder = newFolder;
  destPanel.SetCurrentPathText();
  destPanel.RefreshListCtrl();
}


int CApp::GetFocusedPanelIndex()
{
  HWND hwnd = ::GetFocus();
  while(true)
  {
    if (hwnd == 0)
      return 0;
    for (int i = 0; i < kNumPanels; i++)
      if ((HWND)_panel[i] == hwnd)
        return i;
    hwnd = GetParent(hwnd);
  }
}
