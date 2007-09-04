// App.h

#ifndef __APP_H
#define __APP_H

#include "Panel.h"
#include "AppState.h"
#include "Windows/Control/ImageList.h"

class CApp;

extern CApp g_App;
extern HWND g_HWND;

const int kNumPanelsMax = 2;

extern void MoveSubWindows(HWND hWnd);

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
  STDMETHOD(DragEnter)(IDataObject * dataObject, DWORD keyState, 
      POINTL pt, DWORD *effect);
  STDMETHOD(DragOver)(DWORD keyState, POINTL pt, DWORD * effect);
  STDMETHOD(DragLeave)();
  STDMETHOD(Drop)(IDataObject * dataObject, DWORD keyState, 
      POINTL pt, DWORD *effect);

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

  NWindows::NControl::CImageList _archiveButtonsImageList;
  NWindows::NControl::CImageList _standardButtonsImageList;

  NWindows::NControl::CReBar _rebar;
  NWindows::NControl::CToolBar _archiveToolBar;
  NWindows::NControl::CToolBar _standardToolBar;

  CDropTarget *_dropTargetSpec;
  CMyComPtr<IDropTarget> _dropTarget;

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

  void CreateOnePanel(int panelIndex, const UString &mainPath, bool &archiveIsOpened, bool &encrypted);
  void Create(HWND hwnd, const UString &mainPath, int xSizes[2], bool &archiveIsOpened, bool &encrypted);
  void Read();
  void Save();
  void Release();


  /*
  void SetFocus(int panelIndex)
    { Panels[panelIndex].SetFocusToList(); }
  */
  void SetFocusToLastItem()
    { Panels[LastFocusedPanel].SetFocusToLastRememberedItem(); }

  int GetFocusedPanelIndex() const { return LastFocusedPanel; }

  bool IsPanelVisible(int index) const { return (NumPanels > 1 || index == LastFocusedPanel); }

  /*
  void SetCurrentIndex()
    { CurrentPanel = GetFocusedPanelIndex(); }
  */

  CApp(): NumPanels(2), LastFocusedPanel(0) {}
  CPanel &GetFocusedPanel()
    { return Panels[GetFocusedPanelIndex()]; }

  // File Menu
  void OpenItem()
    { GetFocusedPanel().OpenSelectedItems(true); }
  void OpenItemInside()
    { GetFocusedPanel().OpenFocusedItemAsInternal(); }
  void OpenItemOutside()
    { GetFocusedPanel().OpenSelectedItems(false); }
  void EditItem()
    { GetFocusedPanel().EditItem(); }
  void Rename()
    { GetFocusedPanel().RenameFile(); }
  void CopyTo()
    { OnCopy(false, false, GetFocusedPanelIndex()); }
  void MoveTo()
    { OnCopy(true, false, GetFocusedPanelIndex()); }
  void Delete(bool toRecycleBin)
    { GetFocusedPanel().DeleteItems(toRecycleBin); }
  void CalculateCrc();
  void Split();
  void Combine();
  void Properties()
    { GetFocusedPanel().Properties(); }
  void Comment()
    { GetFocusedPanel().ChangeComment(); }

  void CreateFolder()
    { GetFocusedPanel().CreateFolder(); }
  void CreateFile()
    { GetFocusedPanel().CreateFile(); }

  // Edit
  void EditCut()
    { GetFocusedPanel().EditCut(); }
  void EditCopy()
    { GetFocusedPanel().EditCopy(); }
  void EditPaste()
    { GetFocusedPanel().EditPaste(); }

  void SelectAll(bool selectMode)
    { GetFocusedPanel().SelectAll(selectMode); }
  void InvertSelection()
    { GetFocusedPanel().InvertSelection(); }
  void SelectSpec(bool selectMode)
    { GetFocusedPanel().SelectSpec(selectMode); }
  void SelectByType(bool selectMode)
    { GetFocusedPanel().SelectByType(selectMode); }

  void RefreshStatusBar()
    { GetFocusedPanel().RefreshStatusBar(); }

  void SetListViewMode(UINT32 index)
    { GetFocusedPanel().SetListViewMode(index); }
  UINT32 GetListViewMode()
    { return  GetFocusedPanel().GetListViewMode(); }

  void SortItemsWithPropID(PROPID propID)
    { GetFocusedPanel().SortItemsWithPropID(propID); }

  void OpenRootFolder()
    { GetFocusedPanel().OpenDrivesFolder(); }
  void OpenParentFolder()
    { GetFocusedPanel().OpenParentFolder(); }
  void FoldersHistory()
    { GetFocusedPanel().FoldersHistory(); }
  void RefreshView()
    { GetFocusedPanel().OnReload(); }
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
  void SetListSettings();
  void SetShowSystemMenu();
  void SwitchOnOffOnePanel();
  bool GetFlatMode() { return Panels[LastFocusedPanel].GetFlatMode(); }
  void ChangeFlatMode() { Panels[LastFocusedPanel].ChangeFlatMode(); }

  void OpenBookmark(int index)
    { GetFocusedPanel().OpenBookmark(index); }
  void SetBookmark(int index)
    { GetFocusedPanel().SetBookmark(index); }

  void ReloadRebar(HWND hwnd);
  void ReloadToolbars();
  void ReadToolbar()
  {
    UINT32 mask = ReadToolbarsMask();
    ShowButtonsLables = ((mask & 1) != 0);
    LargeButtons = ((mask & 2) != 0);
    ShowStandardToolbar = ((mask & 4) != 0);
    ShowArchiveToolbar  = ((mask & 8) != 0);
  }
  void SaveToolbar()
  {
    UINT32 mask = 0;
    if (ShowButtonsLables) mask |= 1;
    if (LargeButtons) mask |= 2;
    if (ShowStandardToolbar) mask |= 4;
    if (ShowArchiveToolbar) mask |= 8;
    SaveToolbarsMask(mask);
  }
  void SwitchStandardToolbar()
  { 
    ShowStandardToolbar = !ShowStandardToolbar;
    SaveToolbar();
    ReloadRebar(g_HWND);
    MoveSubWindows(_window);
  }
  void SwitchArchiveToolbar()
  { 
    ShowArchiveToolbar = !ShowArchiveToolbar;
    SaveToolbar();
    ReloadRebar(g_HWND);
    MoveSubWindows(_window);
  }
  void SwitchButtonsLables()
  { 
    ShowButtonsLables = !ShowButtonsLables;
    SaveToolbar();
    ReloadRebar(g_HWND);
    MoveSubWindows(_window);
  }
  void SwitchLargeButtons()
  { 
    LargeButtons = !LargeButtons;
    SaveToolbar();
    ReloadRebar(g_HWND);
    MoveSubWindows(_window);
  }


  void AddToArchive()
    { GetFocusedPanel().AddToArchive(); }
  void ExtractArchives()
    { GetFocusedPanel().ExtractArchives(); }
  void TestArchives()
    { GetFocusedPanel().TestArchives(); }

  void OnNotify(int ctrlID, LPNMHDR pnmh);

  UString PrevTitle;
  void RefreshTitle(bool always = false);
  void RefreshTitleAlways() { RefreshTitle(true); }
  void RefreshTitle(int panelIndex, bool always = false);
};

#endif
