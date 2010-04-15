// App.h

#ifndef __APP_H
#define __APP_H

#include "Windows/Control/CommandBar.h"
#include "Windows/Control/ImageList.h"

#include "AppState.h"
#include "Panel.h"

class CApp;

extern CApp g_App;
extern HWND g_HWND;

const int kNumPanelsMax = 2;

extern bool g_IsSmallScreen;

enum
{
  kAddCommand = kToolbarStartID,
  kExtractCommand,
  kTestCommand
};

class CPanelCallbackImp: public CPanelCallback
{
  CApp *_app;
  int _index;
public:
  void Init(CApp *app, int index)
  {
    _app = app;
    _index = index;
  }
  virtual void OnTab();
  virtual void SetFocusToPath(int index);
  virtual void OnCopy(bool move, bool copyToSame);
  virtual void OnSetSameFolder();
  virtual void OnSetSubFolder();
  virtual void PanelWasFocused();
  virtual void DragBegin();
  virtual void DragEnd();
  virtual void RefreshTitle(bool always);
};

class CApp;

class CDropTarget:
  public IDropTarget,
  public CMyUnknownImp
{
  CMyComPtr<IDataObject> m_DataObject;
  UStringVector m_SourcePaths;
  int m_SelectionIndex;
  bool m_DropIsAllowed;      // = true, if data contain fillist
  bool m_PanelDropIsAllowed; // = false, if current target_panel is source_panel.
                             // check it only if m_DropIsAllowed == true
  int m_SubFolderIndex;
  UString m_SubFolderName;

  CPanel *m_Panel;
  bool m_IsAppTarget;        // true, if we want to drop to app window (not to panel).

  bool m_SetPathIsOK;

  bool IsItSameDrive() const;

  void QueryGetData(IDataObject *dataObject);
  bool IsFsFolderPath() const;
  DWORD GetEffect(DWORD keyState, POINTL pt, DWORD allowedEffect);
  void RemoveSelection();
  void PositionCursor(POINTL ptl);
  UString GetTargetPath() const;
  bool SetPath(bool enablePath) const;
  bool SetPath();

public:
  MY_UNKNOWN_IMP1_MT(IDropTarget)
  STDMETHOD(DragEnter)(IDataObject * dataObject, DWORD keyState, POINTL pt, DWORD *effect);
  STDMETHOD(DragOver)(DWORD keyState, POINTL pt, DWORD * effect);
  STDMETHOD(DragLeave)();
  STDMETHOD(Drop)(IDataObject * dataObject, DWORD keyState, POINTL pt, DWORD *effect);

  CDropTarget():
      TargetPanelIndex(-1),
      SrcPanelIndex(-1),
      m_IsAppTarget(false),
      m_Panel(0),
      App(0),
      m_PanelDropIsAllowed(false),
      m_DropIsAllowed(false),
      m_SelectionIndex(-1),
      m_SubFolderIndex(-1),
      m_SetPathIsOK(false) {}

  CApp *App;
  int SrcPanelIndex;              // index of D&D source_panel
  int TargetPanelIndex;           // what panel to use as target_panel of Application
};

class CApp
{
public:
  NWindows::CWindow _window;
  bool ShowSystemMenu;
  int NumPanels;
  int LastFocusedPanel;

  bool ShowStandardToolbar;
  bool ShowArchiveToolbar;
  bool ShowButtonsLables;
  bool LargeButtons;

  CAppState AppState;
  CPanelCallbackImp m_PanelCallbackImp[kNumPanelsMax];
  CPanel Panels[kNumPanelsMax];
  bool PanelsCreated[kNumPanelsMax];

  NWindows::NControl::CImageList _buttonsImageList;

  #ifdef UNDER_CE
  NWindows::NControl::CCommandBar _commandBar;
  #endif
  NWindows::NControl::CToolBar _toolBar;

  CDropTarget *_dropTargetSpec;
  CMyComPtr<IDropTarget> _dropTarget;

  CApp(): _window(0), NumPanels(2), LastFocusedPanel(0) {}

  void CreateDragTarget()
  {
    _dropTargetSpec = new CDropTarget();
    _dropTarget = _dropTargetSpec;
    _dropTargetSpec->App = (this);
  }

  void SetFocusedPanel(int index)
  {
    LastFocusedPanel = index;
    _dropTargetSpec->TargetPanelIndex = LastFocusedPanel;
  }

  void DragBegin(int panelIndex)
  {
    _dropTargetSpec->TargetPanelIndex = (NumPanels > 1) ? 1 - panelIndex : panelIndex;
    _dropTargetSpec->SrcPanelIndex = panelIndex;
  }

  void DragEnd()
  {
    _dropTargetSpec->TargetPanelIndex = LastFocusedPanel;
    _dropTargetSpec->SrcPanelIndex = -1;
  }

  
  void OnCopy(bool move, bool copyToSame, int srcPanelIndex);
  void OnSetSameFolder(int srcPanelIndex);
  void OnSetSubFolder(int srcPanelIndex);

  HRESULT CreateOnePanel(int panelIndex, const UString &mainPath, const UString &arcFormat, bool &archiveIsOpened, bool &encrypted);
  HRESULT Create(HWND hwnd, const UString &mainPath, const UString &arcFormat, int xSizes[2], bool &archiveIsOpened, bool &encrypted);
  void Read();
  void Save();
  void Release();

  // void SetFocus(int panelIndex) { Panels[panelIndex].SetFocusToList(); }
  void SetFocusToLastItem() { Panels[LastFocusedPanel].SetFocusToLastRememberedItem(); }
  int GetFocusedPanelIndex() const { return LastFocusedPanel; }
  bool IsPanelVisible(int index) const { return (NumPanels > 1 || index == LastFocusedPanel); }
  CPanel &GetFocusedPanel() { return Panels[GetFocusedPanelIndex()]; }

  // File Menu
  void OpenItem() { GetFocusedPanel().OpenSelectedItems(true); }
  void OpenItemInside() { GetFocusedPanel().OpenFocusedItemAsInternal(); }
  void OpenItemOutside() { GetFocusedPanel().OpenSelectedItems(false); }
  void EditItem() { GetFocusedPanel().EditItem(); }
  void Rename() { GetFocusedPanel().RenameFile(); }
  void CopyTo() { OnCopy(false, false, GetFocusedPanelIndex()); }
  void MoveTo() { OnCopy(true, false, GetFocusedPanelIndex()); }
  void Delete(bool toRecycleBin) { GetFocusedPanel().DeleteItems(toRecycleBin); }
  void CalculateCrc();
  void DiffFiles();
  void Split();
  void Combine();
  void Properties() { GetFocusedPanel().Properties(); }
  void Comment() { GetFocusedPanel().ChangeComment(); }

  void CreateFolder() { GetFocusedPanel().CreateFolder(); }
  void CreateFile() { GetFocusedPanel().CreateFile(); }

  // Edit
  void EditCut() { GetFocusedPanel().EditCut(); }
  void EditCopy() { GetFocusedPanel().EditCopy(); }
  void EditPaste() { GetFocusedPanel().EditPaste(); }

  void SelectAll(bool selectMode) { GetFocusedPanel().SelectAll(selectMode); }
  void InvertSelection() { GetFocusedPanel().InvertSelection(); }
  void SelectSpec(bool selectMode) { GetFocusedPanel().SelectSpec(selectMode); }
  void SelectByType(bool selectMode) { GetFocusedPanel().SelectByType(selectMode); }

  void RefreshStatusBar() { GetFocusedPanel().RefreshStatusBar(); }

  void SetListViewMode(UInt32 index) { GetFocusedPanel().SetListViewMode(index); }
  UInt32 GetListViewMode() { return GetFocusedPanel().GetListViewMode(); }
  PROPID GetSortID() { return GetFocusedPanel().GetSortID(); }

  void SortItemsWithPropID(PROPID propID) { GetFocusedPanel().SortItemsWithPropID(propID); }

  void OpenRootFolder() { GetFocusedPanel().OpenDrivesFolder(); }
  void OpenParentFolder() { GetFocusedPanel().OpenParentFolder(); }
  void FoldersHistory() { GetFocusedPanel().FoldersHistory(); }
  void RefreshView() { GetFocusedPanel().OnReload(); }
  void RefreshAllPanels()
  {
    for (int i = 0; i < NumPanels; i++)
    {
      int index = i;
      if (NumPanels == 1)
        index = LastFocusedPanel;
      Panels[index].OnReload();
    }
  }

  /*
  void SysIconsWereChanged()
  {
    for (int i = 0; i < NumPanels; i++)
    {
      int index = i;
      if (NumPanels == 1)
        index = LastFocusedPanel;
      Panels[index].SysIconsWereChanged();
    }
  }
  */

  void SetListSettings();
  void SetShowSystemMenu();
  HRESULT SwitchOnOffOnePanel();
  bool GetFlatMode() { return Panels[LastFocusedPanel].GetFlatMode(); }
  void ChangeFlatMode() { Panels[LastFocusedPanel].ChangeFlatMode(); }

  void OpenBookmark(int index) { GetFocusedPanel().OpenBookmark(index); }
  void SetBookmark(int index) { GetFocusedPanel().SetBookmark(index); }

  void ReloadToolbars();
  void ReadToolbar()
  {
    UInt32 mask = ReadToolbarsMask();
    if (mask & ((UInt32)1 << 31))
    {
      ShowButtonsLables = !g_IsSmallScreen;
      LargeButtons = false;
      ShowStandardToolbar = ShowArchiveToolbar = true;
    }
    else
    {
      ShowButtonsLables = ((mask & 1) != 0);
      LargeButtons = ((mask & 2) != 0);
      ShowStandardToolbar = ((mask & 4) != 0);
      ShowArchiveToolbar  = ((mask & 8) != 0);
    }
  }
  void SaveToolbar()
  {
    UInt32 mask = 0;
    if (ShowButtonsLables) mask |= 1;
    if (LargeButtons) mask |= 2;
    if (ShowStandardToolbar) mask |= 4;
    if (ShowArchiveToolbar) mask |= 8;
    SaveToolbarsMask(mask);
  }
  
  void SaveToolbarChanges();

  void SwitchStandardToolbar()
  {
    ShowStandardToolbar = !ShowStandardToolbar;
    SaveToolbarChanges();
  }
  void SwitchArchiveToolbar()
  {
    ShowArchiveToolbar = !ShowArchiveToolbar;
    SaveToolbarChanges();
  }
  void SwitchButtonsLables()
  {
    ShowButtonsLables = !ShowButtonsLables;
    SaveToolbarChanges();
  }
  void SwitchLargeButtons()
  {
    LargeButtons = !LargeButtons;
    SaveToolbarChanges();
  }

  void AddToArchive() { GetFocusedPanel().AddToArchive(); }
  void ExtractArchives() { GetFocusedPanel().ExtractArchives(); }
  void TestArchives() { GetFocusedPanel().TestArchives(); }

  void OnNotify(int ctrlID, LPNMHDR pnmh);

  UString PrevTitle;
  void RefreshTitle(bool always = false);
  void RefreshTitleAlways() { RefreshTitle(true); }
  void RefreshTitle(int panelIndex, bool always = false);

  void MoveSubWindows();
};

#endif
