// App.h

#pragma once

#ifndef __APP_H
#define __APP_H

#include "Panel.h"
#include "AppState.h"

class CApp;

const kNumPanels = 2;

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
  virtual void OnSetFocusToPath(int index);
  virtual void OnCopy(bool move, bool copyToSame);
  virtual void OnSetSameFolder();
  virtual void OnSetSubFolder();
}; 

class CApp
{
  NWindows::CWindow _window;
public:
  CAppState _appState;
  CPanelCallbackImp m_PanelCallbackImp[kNumPanels];
  CPanel _panel[kNumPanels];
  
  void OnCopy(bool move, bool copyToSame, int srcPanelIndex);
  void OnSetSameFolder(int srcPanelIndex);
  void OnSetSubFolder(int srcPanelIndex);

  void Create(HWND hwnd, UString &path);
  void Read();
  void Save();

  int GetFocusedPanelIndex();

  // File Menu
  void OpenItem()
    { _panel[GetFocusedPanelIndex()].OpenSelectedItems(true); }
  void OpenItemInside()
    { _panel[GetFocusedPanelIndex()].OpenFocusedItemAsInternal(); }
  void OpenItemOutside()
    { _panel[GetFocusedPanelIndex()].OpenSelectedItems(false); }
  void EditItem()
    { _panel[GetFocusedPanelIndex()].EditItem(); }
  void Rename()
    { _panel[GetFocusedPanelIndex()].RenameFile(); }
  void CopyTo()
    { OnCopy(false, false, GetFocusedPanelIndex()); }
  void MoveTo()
    { OnCopy(true, false, GetFocusedPanelIndex()); }
  void Delete()
    { _panel[GetFocusedPanelIndex()].DeleteItems(); }

  void CreateFolder()
    { _panel[GetFocusedPanelIndex()].CreateFolder(); }
  void CreateFile()
    { _panel[GetFocusedPanelIndex()].CreateFile(); }

  // Edit
  void SelectAll(bool selectMode)
    { _panel[GetFocusedPanelIndex()].SelectAll(selectMode); }
  void InvertSelection()
    { _panel[GetFocusedPanelIndex()].InvertSelection(); }
  void SelectSpec(bool selectMode)
    { _panel[GetFocusedPanelIndex()].SelectSpec(selectMode); }
  void SelectByType(bool selectMode)
    { _panel[GetFocusedPanelIndex()].SelectByType(selectMode); }

  void SetListViewMode(UINT32 index)
    { _panel[GetFocusedPanelIndex()].SetListViewMode(index); }
  UINT32 GetListViewMode()
    { return  _panel[GetFocusedPanelIndex()].GetListViewMode(); }

  void SortItemsWithPropID(PROPID propID)
    { _panel[GetFocusedPanelIndex()].SortItemsWithPropID(propID); }

  void OpenRootFolder()
    { _panel[GetFocusedPanelIndex()].OpenDrivesFolder(); }
  void OpenParentFolder()
    { _panel[GetFocusedPanelIndex()].OpenParentFolder(); }
  void FoldersHistory()
    { _panel[GetFocusedPanelIndex()].FoldersHistory(); }
  void RefreshView()
    { _panel[GetFocusedPanelIndex()].OnReload(); }
};

#endif
