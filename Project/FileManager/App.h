// App.h

#pragma once

#ifndef __APP_H
#define __APP_H

#include "Panel.h"
#include "AppState.h"

class CApp;

extern CApp g_App;

const int kNumPanelsMax = 2;

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
}; 

class CApp
{
  NWindows::CWindow _window;
public:
  bool ShowSystemMenu;
  int NumPanels;
  int LastFocusedPanel;

  CAppState _appState;
  CPanelCallbackImp m_PanelCallbackImp[kNumPanelsMax];
  CPanel Panels[kNumPanelsMax];
  bool PanelsCreated[kNumPanelsMax];
  
  void OnCopy(bool move, bool copyToSame, int srcPanelIndex);
  void OnSetSameFolder(int srcPanelIndex);
  void OnSetSubFolder(int srcPanelIndex);

  void CreateOnePanel(int panelIndex, const UString &mainPath);
  void Create(HWND hwnd, const UString &mainPath);
  void Read();
  void Save();

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
    { OnCopy(false, false, GetFocusedPanelIndex()); }
  void MoveTo()
    { OnCopy(true, false, GetFocusedPanelIndex()); }
  void Delete()
    { GetFocusedPanel().DeleteItems(); }
  void Properties()
    { GetFocusedPanel().Properties(); }

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
      if (g_App.NumPanels == 1)
        index = LastFocusedPanel;
      Panels[index].OnReload();
    }
  }
  void SetListSettings();
  void SetShowSystemMenu();
  void SwitchOnOffOnePanel();
};

#endif
