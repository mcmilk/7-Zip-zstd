// ZipViewObject.h

#pragma once

#ifndef __ZIPVIEWOBJECT_H
#define __ZIPVIEWOBJECT_H

#include "Windows/Control/ListView.h"
// #include "ProxyHandler.h"
#include "Windows/Control/ImageList.h"
#include "ExtractDialog.h"
#include "ZipFolder.h"
#include "MyMessages.h"
#include "ProxyHandler.h"

// {23170F69-40C1-278A-0000-000110000000}
DEFINE_GUID(IID_IColumnInfoTransfer, 
0x23170F69, 0x40C1, 0x278A, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);
MIDL_INTERFACE("23170F69-40C1-278A-0000-000110000000")
IColumnInfoTransfer : public IUnknown
{
public:
  // out: if (aProcessedSize == 0) then there are no more bytes
  STDMETHOD(GetListViewInfo)(NZipSettings::CListViewInfo &aListViewInfo) = 0;
};

class CZipViewObject: 
  public IShellView,
  public IColumnInfoTransfer,
  public CComObjectRoot
{
private:
  NWindows::NControl::CListView m_ListView;
  NWindows::NControl::CImageList m_LargeImageList;
  NWindows::NControl::CImageList m_SmallImageList;

  bool m_RedrawEnabled;

  // UINT64 m_NumSelectedItems;
  UINT64 m_SelectedItemsSize;
  UINT32 m_NumSelectedFiles;

  UINT64 m_DirectorySize;

  CSysString m_NSelectedItemsFormat;
  CSysString m_NObjectsFormat;
  CSysString m_FileSizeFormat;



  typedef std::map<UString, int> CUStringToIntMap;
  CUStringToIntMap m_ExtensionToIconIndexMap;
  int m_DirIconIndex;
  typedef std::map<UString, CSysString> CUStringToCSysString;
  CUStringToCSysString m_ExtensionToFileTypeMap;
  CSysString m_DirFileTypeString;
  bool m_DirFileTypeStringDefined;

  CRecordVector<PROPID> m_ColumnsPropIDs;
  NZipSettings::CListViewInfo m_RegistryListViewInfo;
  
  void AddCulumnItemInfo(const NZipSettings::CListViewInfo &aListViewInfo,
      PROPID aPropID);
  
  // CSysString GetNameOfItem(PROPID anID);
  void onColumnCommand();

  // CSysString GetFileType(CDirOrFileIndex aDirOrFileIndex);
  CSysString GetFileType(UINT32 anIndex);

  LRESULT SetItemText(LV_DISPINFO *aDispInfo, UINT32 anIndex);
  LRESULT SetIcon(LV_DISPINFO *aDispInfo, UINT32 anIndex);


  void InitListCtrl();
  void RefreshListCtrl();
  void OnViewStyle(UINT uiStyle);
  void SendRefreshStatusBarMessage();
  void ShowStatusBar();
  void SelChange(NMLISTVIEW *pNMLV);

  static LRESULT CALLBACK NameSpaceWndProc(HWND hWnd, UINT uMessage, 
                        WPARAM wParam, LPARAM lParam);
  static int CALLBACK CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);
  static int CALLBACK CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lpData);
private:
  // Windows Message Handlers
  LRESULT onPaint(HDC hDC);
  LRESULT onEraseBkGnd(HDC hDC);
  LRESULT onCreate(void);
  LRESULT onSize(WORD, WORD);
  LRESULT onNotify(UINT, LPNMHDR);
  bool OnContextMenu(HANDLE aWindowHandle, int xPos, int yPos);

  LRESULT onKeyDown(LPNMLVKEYDOWN aKeyDownInfo);
  LRESULT onColumnClick(LPNMLISTVIEW aListView);
  LRESULT onRightClick(LPNMITEMACTIVATE anItemActiveate);

  void CommandExtract();
  HRESULT ExtractItems(const NExtractionDialog::CModeInfo &anExtractModeInfo,
      const CSysString &aDirectoryPath,
      const CRecordVector<UINT32> &aLocalIndexes,
      bool aPasswordIsDefined, const UString &aPassword);

  // void CommandDelete();
  void CommandHelp();

  LRESULT onCommand(HWND, DWORD dwNotifyCode, DWORD dwID);
  LRESULT onSetFocus(HWND hWndOld);
  LRESULT onKillFocus(HWND hWndOld);

protected:
  CProxyHandler *m_ProxyHandler;
  CComPtr<IArchiveFolder> m_ArchiveFolder;

  CZipFolder *m_ZipFolder;
  CComPtr<IShellFolder> m_ShellFolder;
  CComPtr<IMalloc> m_Malloc;


  CItemIDListManager *m_IDListManager;

  NWindows::CWindow m_Window;

  LPSHELLBROWSER m_ShellBrowser;
  HWND m_hWndParent; // ShellBrowser Window
  FOLDERSETTINGS  m_FolderSettings;
  
    HWND m_hwndMain;
    HMENU m_hMenu;
    // CHtmlTree *m_pHtmlTree;

    VOID EnableUI( VOID );

  // LPITEMIDLIST  m_IDList;
  RECT      m_rect;

  void MessageBox(LPCTSTR aMessage)
    { ::MyMessageBox(m_Window, aMessage); }

public:
  CZipViewObject(); 
  ~CZipViewObject();

BEGIN_COM_MAP(CZipViewObject)
  COM_INTERFACE_ENTRY(IShellView)
  COM_INTERFACE_ENTRY(IColumnInfoTransfer)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CZipViewObject)

DECLARE_NO_REGISTRY()

  //IOleWindow methods
  STDMETHOD (GetWindow) (HWND*);
  STDMETHOD (ContextSensitiveHelp) (BOOL);

  //IShellView methods
  STDMETHOD (TranslateAccelerator) (LPMSG);
  STDMETHOD (EnableModeless) (BOOL);
  STDMETHOD (UIActivate) (UINT);
  STDMETHOD (Refresh) (void);
  STDMETHOD (CreateViewWindow) (LPSHELLVIEW, LPCFOLDERSETTINGS, LPSHELLBROWSER, 
                LPRECT, HWND*);
  STDMETHOD (DestroyViewWindow) (void);
  STDMETHOD (GetCurrentInfo) (LPFOLDERSETTINGS);
  STDMETHOD (AddPropertySheetPages) (DWORD, LPFNADDPROPSHEETPAGE, LPARAM);
  STDMETHOD (SaveViewState) (void);
  STDMETHOD (SelectItem) (LPCITEMIDLIST, UINT);
  STDMETHOD (GetItemObject) (UINT, REFIID, LPVOID*);

  // IColumnInfoTransfer
  STDMETHOD(GetListViewInfo)(NZipSettings::CListViewInfo &aListViewInfo);

  void Init(CZipFolder *aFolder, /*LPCITEMIDLIST anIDList,*/
      CProxyHandler *aProxyHandler,
      IArchiveFolder *anArchiveFolderItem);
  void SetFolderItem(IArchiveFolder *anArchiveFolderItem);

  void RestoreViewState();
  void MergeToolBar();
  // void StartItem();
  void OpenFolder(UINT32 anIndex);
  void OpenItem(UINT32 anIndex);
  void OpenSelectedItems();
  void GetSelectedItemsIndexes(CRecordVector<UINT32> &anIndexes);
};



#endif