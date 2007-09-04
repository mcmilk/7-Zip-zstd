// Panel.h

#ifndef __PANEL_H
#define __PANEL_H

#include "Common/MyCom.h"

#include "Windows/DLL.h"
#include "Windows/FileFind.h"
#include "Windows/FileDir.h"
#include "Windows/Synchronization.h"
#include "Windows/Handle.h"

#include "Windows/Control/ToolBar.h"
#include "Windows/Control/ReBar.h"
#include "Windows/Control/ListView.h"
#include "Windows/Control/Static.h"
#include "Windows/Control/Edit.h"
#include "Windows/Control/ComboBox.h"
#include "Windows/Control/Window2.h"
#include "Windows/Control/StatusBar.h"

#include "SysIconUtils.h"
#include "IFolder.h"
#include "ViewSettings.h"
#include "AppState.h"
#include "MyCom2.h"

const int kParentFolderID = 100;
const int kPluginMenuStartID = 1000;
const int kToolbarStartID = 2000;

const int kParentIndex = -1;

struct CPanelCallback
{
  virtual void OnTab() = 0;
  virtual void SetFocusToPath(int index) = 0;
  virtual void OnCopy(bool move, bool copyToSame) = 0;
  virtual void OnSetSameFolder() = 0;
  virtual void OnSetSubFolder() = 0;
  virtual void PanelWasFocused() = 0;
  virtual void DragBegin() = 0;
  virtual void DragEnd() = 0;
  virtual void RefreshTitle(bool always) = 0;
};

void PanelCopyItems();

struct CItemProperty
{
  UString Name;
  PROPID ID;
  VARTYPE Type;
  int Order;
  bool IsVisible;
  UInt32 Width;
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
  UString FolderPath;
  UString FilePath;
  NWindows::NFile::NFind::CFileInfoW FileInfo;
  void DeleteDirAndFile()
  {
    NWindows::NFile::NDirectory::DeleteFileAlways(FilePath);
    NWindows::NFile::NDirectory::MyRemoveDirectory(FolderPath);
  }
};

struct CFolderLink: public CTempFileInfo
{
  NWindows::NDLL::CLibrary Library;
  CMyComPtr<IFolderFolder> ParentFolder;
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

/*
class CMyComboBox: public NWindows::NControl::CComboBoxEx
{
public:
  WNDPROC _origWindowProc;
  CPanel *_panel;
  LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
};
*/
class CMyComboBoxEdit: public NWindows::NControl::CEdit
{
public:
  WNDPROC _origWindowProc;
  CPanel *_panel;
  LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

struct CSelectedState
{
  int FocusedItem;
  UString FocusedName;
  bool SelectFocused;
  UStringVector SelectedNames;
  CSelectedState(): FocusedItem(-1), SelectFocused(false) {}
};

class CPanel:public NWindows::NControl::CWindow2
{
  HWND _mainWindow;

  CExtToIconMap _extToIconMap;
  UINT _baseID;
  int _comboBoxID;
  UINT _statusBarID;

  CAppState *_appState;

  bool OnCommand(int code, int itemID, LPARAM lParam, LRESULT &result);
  LRESULT OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
  virtual bool OnCreate(CREATESTRUCT *createStruct);
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  virtual void OnDestroy();
  virtual bool OnNotify(UINT controlID, LPNMHDR lParam, LRESULT &result);
  void OnComboBoxCommand(UINT code, LPARAM &aParam);
  bool OnNotifyComboBoxEndEdit(PNMCBEENDEDITW info, LRESULT &result);
  #ifndef _UNICODE
  bool OnNotifyComboBoxEndEdit(PNMCBEENDEDIT info, LRESULT &result);
  #endif
  bool OnNotifyReBar(LPNMHDR lParam, LRESULT &result);
  bool OnNotifyComboBox(LPNMHDR lParam, LRESULT &result);
  void OnItemChanged(NMLISTVIEW *item);
  bool OnNotifyList(LPNMHDR lParam, LRESULT &result);
  void OnDrag(LPNMLISTVIEW nmListView);
  bool OnKeyDown(LPNMLVKEYDOWN keyDownInfo, LRESULT &result);
  BOOL OnBeginLabelEdit(LV_DISPINFOW * lpnmh);
  BOOL OnEndLabelEdit(LV_DISPINFOW * lpnmh);
  void OnColumnClick(LPNMLISTVIEW info);
  bool OnCustomDraw(LPNMLVCUSTOMDRAW lplvcd, LRESULT &result);

public:
  CPanelCallback *_panelCallback;

  void DeleteItems(bool toRecycleBin);
  void DeleteItemsInternal(CRecordVector<UInt32> &indices);
  void CreateFolder();
  void CreateFile();

private:

  void ChangeWindowSize(int xSize, int ySize);
 
  void InitColumns();
  // void InitColumns2(PROPID sortID);
  void InsertColumn(int index);

  void SetFocusedSelectedItem(int index, bool select);
  void RefreshListCtrl(const UString &focusedName, int focusedPos, bool selectFocused,
      const UStringVector &selectedNames);

  void OnShiftSelectMessage();
  void OnArrowWithShift();

  void OnInsert();
  // void OnUpWithShift();
  // void OnDownWithShift();
public:
  void UpdateSelection();
  void SelectSpec(bool selectMode);
  void SelectByType(bool selectMode);
  void SelectAll(bool selectMode);
  void InvertSelection();
private:

  // UString GetFileType(UInt32 index);
  LRESULT SetItemText(LVITEMW &item);

  // CRecordVector<PROPID> m_ColumnsPropIDs;

public:
  NWindows::NControl::CReBar _headerReBar;
  NWindows::NControl::CToolBar _headerToolBar;
  NWindows::NControl::CComboBoxEx _headerComboBox;
  // CMyComboBox _headerComboBox;
  CMyComboBoxEdit _comboBoxEdit;
  CMyListView _listView;
  NWindows::NControl::CStatusBar _statusBar;
  bool _lastFocusedIsList;
  // NWindows::NControl::CStatusBar _statusBar2;

  DWORD _exStyle;
  bool _showDots;
  bool _showRealFileIcons;
  // bool _virtualMode;
  // CUIntVector _realIndices;
  bool _enableItemChangeNotify;
  bool _mySelectMode;
  CBoolVector _selectedStatusVector;

  CSelectedState _selectedState;

  UInt32 GetRealIndex(const LVITEMW &item) const
  {
    /*
    if (_virtualMode)
      return _realIndices[item.iItem];
    */
    return (UInt32)item.lParam;
  }
  int GetRealItemIndex(int indexInListView) const
  {
    /*
    if (_virtualMode)
      return indexInListView;
    */
    LPARAM param;
    if (!_listView.GetItemParam(indexInListView, param))
      throw 1;
    return (int)param;
  }

  UInt32 _ListViewMode;
  int _xSize; 

  bool _flatMode;

  bool _dontShowMode;


  UString _currentFolderPrefix;
  
  CObjectVector<CFolderLink> _parentFolders;
  NWindows::NDLL::CLibrary _library;
  CMyComPtr<IFolderFolder> _folder;
  // CMyComPtr<IFolderGetSystemIconIndex> _folderGetSystemIconIndex;

  UStringVector _fastFolders;

  void GetSelectedNames(UStringVector &selectedNames);
  void SaveSelectedState(CSelectedState &s);
  void RefreshListCtrl(const CSelectedState &s);
  void RefreshListCtrlSaveFocused();

  UString GetItemName(int itemIndex) const;
  UString GetItemPrefix(int itemIndex) const;
  UString GetItemRelPath(int itemIndex) const;
  bool IsItemFolder(int itemIndex) const;
  UInt64 GetItemSize(int itemIndex) const;

  ////////////////////////
  // PanelFolderChange.cpp

  void SetToRootFolder();
  HRESULT BindToPath(const UString &fullPath, bool &archiveIsOpened, bool &encrypted); // can be prefix 
  HRESULT BindToPathAndRefresh(const UString &path);
  void OpenDrivesFolder();
  
  void SetBookmark(int index);
  void OpenBookmark(int index);
  
  void LoadFullPath();
  void LoadFullPathAndShow();
  void FoldersHistory();
  void OpenParentFolder();
  void CloseOpenFolders();
  void OpenRootFolder();


  LRESULT Create(HWND mainWindow, HWND parentWindow, 
      UINT id,
      const UString &currentFolderPrefix, 
      CPanelCallback *panelCallback,
      CAppState *appState, bool &archiveIsOpened, bool &encrypted);
  void SetFocusToList();
  void SetFocusToLastRememberedItem();


  void ReadListViewInfo();
  void SaveListViewInfo();

  CPanel() : 
      // _virtualMode(flase),
      _exStyle(0),
      _showDots(false),
      _showRealFileIcons(false),
      _needSaveInfo(false), 
      _startGroupSelect(0), 
      _selectionIsDefined(false),
      _ListViewMode(3),
      _flatMode(false),
      _xSize(300),
      _mySelectMode(false),
      _enableItemChangeNotify(true),
      _dontShowMode(false)
  {} 

  void SetExtendedStyle()
  {
    if (_listView != 0)
      _listView.SetExtendedListViewStyle(_exStyle);
  }


  bool _needSaveInfo;
  UString _typeIDString;
  CListViewInfo _listViewInfo;
  CItemProperties _properties;
  CItemProperties _visibleProperties;
  
  PROPID _sortID;
  // int _sortIndex;
  bool _ascending;

  void Release();
  ~CPanel();
  void OnLeftClick(LPNMITEMACTIVATE itemActivate);
  bool OnRightClick(LPNMITEMACTIVATE itemActivate, LRESULT &result);

  void OnTimer();
  void OnReload();
  bool OnContextMenu(HANDLE windowHandle, int xPos, int yPos);

  CMyComPtr<IContextMenu> _sevenZipContextMenu;
  CMyComPtr<IContextMenu> _systemContextMenu;
  HRESULT CreateShellContextMenu(
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &systemContextMenu);
  void CreateSystemMenu(HMENU menu, 
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &systemContextMenu);
  void CreateSevenZipMenu(HMENU menu, 
      const CRecordVector<UInt32> &operatedIndices,
      CMyComPtr<IContextMenu> &sevenZipContextMenu);
  void CreateFileMenu(HMENU menu, 
      CMyComPtr<IContextMenu> &sevenZipContextMenu,
      CMyComPtr<IContextMenu> &systemContextMenu,
      bool programMenu);
  void CreateFileMenu(HMENU menu);
  bool InvokePluginCommand(int id);
  bool InvokePluginCommand(int id, IContextMenu *sevenZipContextMenu, 
      IContextMenu *systemContextMenu);

  void InvokeSystemCommand(const char *command);
  void Properties();
  void EditCut();
  void EditCopy();
  void EditPaste();

  int _startGroupSelect;

  bool _selectionIsDefined;
  bool _selectMark;
  int _prevFocusedItem;

 
  // void SortItems(int index);
  void SortItemsWithPropID(PROPID propID);

  void GetSelectedItemsIndices(CRecordVector<UInt32> &indices) const;
  void GetOperatedItemIndices(CRecordVector<UInt32> &indices) const;
  void GetAllItemIndices(CRecordVector<UInt32> &indices) const;
  // void GetOperatedListViewIndices(CRecordVector<UInt32> &indices) const;
  void KillSelection();

  UString GetFolderTypeID() const;
  bool IsRootFolder() const;
  bool IsFSFolder() const;
  bool IsFSDrivesFolder() const;

  UString GetFsPath() const;
  UString GetDriveOrNetworkPrefix() const;

  bool DoesItSupportOperations() const;

  bool _processTimer;
  bool _processNotify;

  class CDisableTimerProcessing
  {
    bool _processTimerMem;
    bool _processNotifyMem;

    CPanel &_panel;
    public:

    CDisableTimerProcessing(CPanel &panel): _panel(panel) 
    { 
      Disable();
    }
    void Disable()
    {
      _processTimerMem = _panel._processTimer;
      _processNotifyMem = _panel._processNotify;
      _panel._processTimer = false; 
      _panel._processNotify = false; 
    }
    void Restore()
    {
      _panel._processTimer = _processTimerMem; 
      _panel._processNotify = _processNotifyMem; 
    }
    ~CDisableTimerProcessing() 
    { 
      Restore();
    }
    CDisableTimerProcessing& operator=(const CDisableTimerProcessing &) {; }
  };

  // bool _passwordIsDefined;
  // UString _password;

  void RefreshListCtrl();

  void MessageBoxInfo(LPCWSTR message, LPCWSTR caption);
  void MessageBox(LPCWSTR message);
  void MessageBox(LPCWSTR message, LPCWSTR caption);
  void MessageBoxMyError(LPCWSTR message);
  void MessageBoxError(HRESULT errorCode, LPCWSTR caption);
  void MessageBoxError(HRESULT errorCode);
  void MessageBoxLastError(LPCWSTR caption);
  void MessageBoxLastError();


  void OpenFocusedItemAsInternal();
  void OpenSelectedItems(bool internal);

  void OpenFolderExternal(int index);

  void OpenFolder(int index);
  HRESULT OpenParentArchiveFolder();
  HRESULT OpenItemAsArchive(const UString &name, 
      const UString &folderPath,
      const UString &filePath, bool &encrypted);
  HRESULT OpenItemAsArchive(const UString &aName);
  HRESULT OpenItemAsArchive(int index);
  void OpenItemInArchive(int index, bool tryInternal, bool tryExternal,
      bool editMode);
  HRESULT OnOpenItemChanged(const UString &folderPath, const UString &itemName);
  LRESULT OnOpenItemChanged(LPARAM lParam);

  void OpenItem(int index, bool tryInternal, bool tryExternal);
  void EditItem();
  void EditItem(int index);

  void RenameFile();
  void ChangeComment();

  void SetListViewMode(UInt32 index);
  UInt32 GetListViewMode() const { return _ListViewMode; };

  void ChangeFlatMode();
  bool GetFlatMode() const { return _flatMode; };

  void RefreshStatusBar();
  void OnRefreshStatusBar();

  void AddToArchive();
  void ExtractArchives();
  void TestArchives();

  HRESULT CopyTo(const CRecordVector<UInt32> &indices, const UString &folder, 
      bool moveMode, bool showErrorMessages, UStringVector *messages);

  HRESULT CopyFrom(const UString &folderPrefix, const UStringVector &filePaths, 
      bool showErrorMessages, UStringVector *messages);

  void CopyFromNoAsk(const UStringVector &filePaths);
  void CopyFromAsk(const UStringVector &filePaths);

  // empty folderPath means create new Archive to path of first fileName.
  void DropObject(IDataObject * dataObject, const UString &folderPath);

  // empty folderPath means create new Archive to path of first fileName.
  void CompressDropFiles(const UStringVector &fileNames, const UString &folderPath);

  void RefreshTitle(bool always = false) { _panelCallback->RefreshTitle(always);  }
  void RefreshTitleAlways() { RefreshTitle(true);  }
};

#endif
