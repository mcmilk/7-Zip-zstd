// Panel.h

#pragma once

#ifndef __PANEL_H
#define __PANEL_H

#include "Windows/Control/Toolbar.h"
#include "Windows/Control/ListView.h"
#include "Windows/Control/Static.h"
#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"
#include "Windows/Control/Window2.h"
#include "Windows/Control/StatusBar.h"

#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Synchronization.h"
#include "Windows/Handle.h"

#include "SysIconUtils.h"

#include "FolderInterface.h"

#include "ViewSettings.h"

#include "AppState.h"

class CPanelCallback
{
public:
  virtual void OnTab() = 0;
  virtual void OnSetFocusToPath(int index) = 0;
  virtual void OnCopy(bool move, bool copyToSame) = 0;
  virtual void OnSetSameFolder() = 0;
  virtual void OnSetSubFolder() = 0;
};

void PanelCopyItems();

struct CItemProperty
{
  UString Name;
  PROPID ID;
  VARTYPE Type;
  int Order;
  bool IsVisible;
  UINT32 Width;
};

inline bool operator<(const CItemProperty &a1, const CItemProperty &a2)
  { return (a1.Order < a2.Order); }

inline bool operator==(const CItemProperty &a1, const CItemProperty &a2)
  { return (a1.Order == a2.Order); }

class CItemProperties: public CObjectVector<CItemProperty>
{
public:
  int FindItemWithID(PROPID id)
  {
    for (int i = 0; i < Size(); i++)
      if ((*this)[i].ID == id)
        return i;
    return -1;
  }
};

struct CTempFileInfo
{
  UString ItemName;
  CSysString FolderPath;
  CSysString FilePath;
  NWindows::NFile::NFind::CFileInfo FileInfo;
  void DeleteDirAndFile()
  {
    NWindows::NFile::NDirectory::DeleteFileAlways(FilePath);
    ::RemoveDirectory(FolderPath);
  }
};

struct CFolderLink: public CTempFileInfo
{
  CComPtr<IFolderFolder> ParentFolder;

  // CSysString RealPath;
};

enum MyMessages
{
  kShiftSelectMessage  = WM_USER + 1,
  kReLoadMessage,
  kSetFocusToListView,
  kOpenItemChanged,
  kRefreshStatusBar
};

UString GetFolderPath(IFolderFolder * folder);

class CPanel;

class CMyListView: public NWindows::NControl::CListView
{
public:
  WNDPROC _origWindowProc;
  CPanel *_panel;
  LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

class CPanel:public NWindows::NControl::CWindow2
{
  CExtToIconMap _extToIconMap;
  int _index;
  UINT _baseID;
  UINT _comboBoxID;
  UINT _statusBarID;

  CPanelCallback *_panelCallback;
  CAppState *_appState;

  bool OnCommand(int code, int itemID, LPARAM lParam, LRESULT &result);
  LRESULT OnMessage(UINT message, UINT wParam, LPARAM lParam);
  virtual bool OnCreate(CREATESTRUCT *createStruct);
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  virtual void OnDestroy();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam, LRESULT &result);
  void OnComboBoxCommand(UINT code, LPARAM &aParam);
  bool OnNotifyComboBoxEndEdit(PNMCBEENDEDIT info, LRESULT &result);
  bool OnNotifyComboBox(LPNMHDR lParam, LRESULT &result);
  bool OnNotifyList(LPNMHDR lParam, LRESULT &result);
  bool OnKeyDown(LPNMLVKEYDOWN keyDownInfo, LRESULT &result);
  BOOL OnBeginLabelEdit(LV_DISPINFO * lpnmh);
  BOOL OnEndLabelEdit(LV_DISPINFO * lpnmh);
  void OnColumnClick(LPNMLISTVIEW info);
  bool OnCustomDraw(LPNMLVCUSTOMDRAW lplvcd, LRESULT &result);


public:
  void DeleteItems();
  void CreateFolder();
  void CreateFile();

private:
 
  void InitColumns();
  // void InitColumns2(PROPID sortID);
  void InsertColumn(int index);

  void RefreshListCtrl(const UString &focusedName, int focusedPos,
      const UStringVector &selectedNames);

  void OnShiftSelectMessage();
  void OnArrowWithShift();

  void OnInsert();
  // void OnUpWithShift();
  // void OnDownWithShift();
public:
  void SelectSpec(bool selectMode);
  void SelectByType(bool selectMode);
  void SelectAll(bool selectMode);
  void InvertSelection();
private:

  CSysString GetFileType(UINT32 index);
  LRESULT SetItemText(LV_DISPINFO *dispInfo, UINT32 index);

  // CRecordVector<PROPID> m_ColumnsPropIDs;

public:
  NWindows::NControl::CToolBar _headerToolBar;
  NWindows::NControl::CComboBoxEx _headerComboBox;
  CMyListView _listView;
  NWindows::NControl::CStatusBar _statusBar;
  // NWindows::NControl::CStatusBar _statusBar2;

  CBoolVector _selectedStatusVector;
  UINT32 _ListViewMode;

  UString _currentFolderPrefix;
  
  CObjectVector<CFolderLink> _parentFolders;
  CComPtr<IFolderFolder> _folder;
  UStringVector _fastFolders;

  void GetSelectedNames(UStringVector &selectedNames);
  void RefreshListCtrlSaveFocused();

  UString GetItemName(int itemIndex) const;
  bool IsItemFolder(int itemIndex) const;
  UINT64 GetItemSize(int itemIndex) const;

  LRESULT Create(HWND parrentWindow, int index, UINT id, int xPos, 
      CSysString &currentFolderPrefix, CPanelCallback *panelCallback,
      CAppState *appState);
  void SetFocus();


  void ReadListViewInfo();
  void SaveListViewInfo();

  CPanel() : _needSaveInfo(false), _startGroupSelect(0), 
      _selectionIsDefined(false){} 

  bool _needSaveInfo;
  CSysString _typeIDString;
  CListViewInfo _listViewInfo;
  CItemProperties _properties;
  CItemProperties _visibleProperties;
  
  int _sortID;
  // int _sortIndex;
  bool _ascending;

  ~CPanel();
  void OnLeftClick(LPNMITEMACTIVATE itemActivate);
  bool OnRightClick(LPNMITEMACTIVATE itemActivate, LRESULT &result);

  void LoadCurrentPath();
  void OnTimer();
  void OnReload();
  void SetToRootFolder();
  bool OnContextMenu(HANDLE windowHandle, int xPos, int yPos);

  int GetRealItemIndex(int indexInListView) const
  {
    LPARAM param;
    if (!_listView.GetItemParam(indexInListView, param))
      throw 1;
    return param;
  }

  void FoldersHistory();

  int _startGroupSelect;

  bool _selectionIsDefined;
  bool _selectMark;
  int _prevFocusedItem;

  HRESULT BindToFolder(const UString &path);
  
  void FastFolderInsert(int index);
  void FastFolderSelect(int index);

  // void SortItems(int index);
  void SortItemsWithPropID(PROPID propID);

  void GetSelectedItemsIndexes(CRecordVector<UINT32> &indices) const;
  void GetOperatedItemIndexes(CRecordVector<UINT32> &indices) const;
  void KillSelection();

  bool IsFSFolder() const;

  bool _processTimer;
  bool _processNotify;

  class CDisableTimerProcessing
  {
    CPanel &_panel;
    public:
      CDisableTimerProcessing(CPanel &panel): _panel(panel) 
      { 
        _panel._processTimer = false; 
        _panel._processNotify = false; 
      }
      ~CDisableTimerProcessing() 
      { 
        _panel._processTimer = true; 
        _panel._processNotify = true; 
      }
  };

  void OpenDrivesFolder();
  void SetCurrentPathText();
  void RefreshListCtrl();

  void MessageBox(LPCWSTR message);
  void MessageBox(LPCWSTR message, LPCWSTR caption);
  void MessageBoxMyError(LPCWSTR message);
  void MessageBoxError(HRESULT errorCode, LPCWSTR caption);
  void MessageBoxLastError(LPCWSTR caption);
  void MessageBoxLastError();


  void OpenFocusedItemAsInternal();
  void OpenSelectedItems(bool internal);

  void OpenFolderExternal(int index);
  void OpenRootFolder();
  void OpenParentFolder();
  void CloseOpenFolders();
  void OpenFolder(int index);
  HRESULT OpenParentArchiveFolder();
  HRESULT OpenItemAsArchive(const UString &name, 
      const CSysString &folderPath,
      const CSysString &filePath);
  HRESULT OpenItemAsArchive(const UString &aName);
  HRESULT OpenItemAsArchive(int index);
  // void OpenItem(int index, CSysString realPath);
  void OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
      bool editMode);
  LRESULT OnOpenItemChanged(const CSysString &folderPath, const UString &itemName);
  LRESULT OnOpenItemChanged(LPARAM lParam);

  void OpenItem(int index, bool tryInternal, bool tryExternal);
  void EditItem();
  void EditItem(int index);

  void RenameFile();

  void SetListViewMode(UINT32 index);
  UINT32 GetListViewMode() { return _ListViewMode; };

  void RefreshStatusBar();
  void OnRefreshStatusBar();
};

#endif
