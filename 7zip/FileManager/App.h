// App.h

#pragma once

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
  virtual void OnCopy(UStringVector &externalNames, bool move, bool copyToSame);
  virtual void OnSetSameFolder();
  virtual void OnSetSubFolder();
  virtual void PanelWasFocused();
}; 

class CApp
{
  NWindows::CWindow _window;
public:
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
  
  void OnCopy(UStringVector &externalNames, 
      bool move, bool copyToSame, int srcPanelIndex);
  void OnSetSameFolder(int srcPanelIndex);
  void OnSetSubFolder(int srcPanelIndex);

  void CreateOnePanel(int panelIndex, const UString &mainPath);
  void Create(HWND hwnd, const UString &mainPath);
  void Read();
  void Save();
  void Release();


  /*
  void SetFocus(int panelIndex)
    { Panels[panelIndex].SetFocusToList(); }
  */
  void SetFocusToLastItem()
    { Panels[LastFocusedPanel].SetFocusToLastRememberedItem(); }

  int GetFocusedPanelIndex();
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
    { OnCopy(UStringVector(), false, false, GetFocusedPanelIndex()); }
  void MoveTo()
    { OnCopy(UStringVector(), true, false, GetFocusedPanelIndex()); }
  void Delete()
    { GetFocusedPanel().DeleteItems(); }
  void Properties()
    { GetFocusedPanel().Properties(); }
  void Comment()
    { GetFocusedPanel().ChangeComment(); }

  void CreateFolder()
    { GetFocusedPanel().CreateFolder(); }
  void CreateFile()
    { GetFocusedPanel().CreateFile(); }

  // Edit
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
  void ExtractArchive()
    { GetFocusedPanel().ExtractArchive(); }
  void TestArchive()
    { GetFocusedPanel().TestArchive(); }

  void OnNotify(int ctrlID, LPNMHDR pnmh);
};

#endif
