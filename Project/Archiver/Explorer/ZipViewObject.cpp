// ZipViewObject.cpp

#include "StdAfx.h"

#include <windowsx.h>

#include "ZipViewObject.h"
#include "ZipViewUtils.h"

#include "resource.h"
#include "ColumnsDialog.h"

#include "../Format/Common/IArchiveHandler.h"
#include "Windows/FileDir.h"
#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/ResourceString.h"
#include "Windows/Menu.h"
#include "Windows/FileFind.h"

#include "Windows/PropVariantConversions.h"

#include "Common/StringConvert.h"

#include "Interface/FileStreams.h"

#include "../Common/HelpUtils.h"
#include "../Common/PropIDUtils.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

#include "FormatUtils.h"

#include "MyIDList.h"

#include "../Resource/OverwriteDialog/resource.h"

using namespace NWindows;

extern CComModule _Module;

static const kDefaultColumnWidth = 50;

#define NAMESPACEVIEW_CLASS   TEXT("ZipView NameSpace Extension_ZipView")

#define IDC_VIEW_ICON                   101 
#define IDC_VIEW_SMALLICON              102 
#define IDC_VIEW_LIST                   103
#define IDC_VIEW_DETAILS                104

#define IDC_VIEW_SORTTYPE 110
#define IDC_VIEW_SORTNAME 111
#define IDC_VIEW_SORTDATE 112
#define IDC_VIEW_SORTSIZE 113

#define IDC_EDIT_PARENTFOLDER 120

#define IDC_EDIT_COPY  130 
// #define IDC_EDIT_DELETE 121 

#define IDC_MY_EXTRACT 140 
//#define IDC_MY_DELETE 141

#define IDC_MY_CONTEXT_MENU_COLUMN 200 
#define IDC_MY_CONTEXT_MENU_OPEN 201 
#define IDC_MY_CONTEXT_MENU_EXTRACT 202 


#define WM_USER_UPDATESTATUS (WM_USER + 1)


CZipViewObject::CZipViewObject():
  m_ShellFolder(NULL),
  m_hWndParent(NULL),
  m_ArchiveFolder(NULL),
  m_DirIconIndex(-1),
  m_DirFileTypeStringDefined(false),
  m_RedrawEnabled(true)
{
  // MessageBox(NULL, "CZipViewObject::CZipViewObject", "", MB_OK);
  if(FAILED(SHGetMalloc(&m_Malloc)))
  {
    // Raise an exception too
    delete this;
    return;
  }
  InitCommonControls();

  m_SmallImageList.Create(GetSystemMetrics(SM_CXSMICON), 
      GetSystemMetrics(SM_CYSMICON), ILC_COLORDDB | ILC_MASK, 4, 4);

  m_LargeImageList.Create(GetSystemMetrics(SM_CXICON), 
      GetSystemMetrics(SM_CYICON), ILC_COLORDDB | ILC_MASK, 4, 4);
  
  m_IDListManager = new CItemIDListManager;
}

void CZipViewObject::SetFolderItem(IArchiveFolder *anArchiveFolderItem)
{
  m_ArchiveFolder = anArchiveFolderItem;
}

void CZipViewObject::Init(CZipFolder *aFolder,
    CProxyHandler *aProxyHandler,
    IArchiveFolder *anArchiveFolderItem)
{
  SetFolderItem(anArchiveFolderItem);
  m_ShellFolder = aFolder;
  m_ZipFolder = aFolder;

  m_ProxyHandler = aProxyHandler;
  
  m_FileSizeFormat = LangLoadString(IDS_FILE_SIZE, 0x02000982);
  m_NSelectedItemsFormat = LangLoadString(IDS_N_SELECTED_ITEMS, 0x02000301);
  m_NObjectsFormat = LangLoadString(IDS_N_OBJECTS, 0x02000302);


  // PrintNumber(m_ProxyHandler->m_Properties.size(), "Init:m_Properties.size()");
  // m_IDList = m_IDListManager->Copy(anIDList);
}

CZipViewObject::~CZipViewObject()
{
  // MessageBox(NULL, "CZipViewObject::~CZipViewObject", "", MB_OK);
  if(m_IDListManager)
    delete m_IDListManager;
}


///////////////////////////////////////////////////////////////////////////
// IOleWindow Implementation
STDMETHODIMP CZipViewObject::GetWindow(HWND *phWnd)
{
  *phWnd = m_Window;
  return S_OK;
}

STDMETHODIMP CZipViewObject::ContextSensitiveHelp(BOOL fEnterMode)
{
  return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
// IShellView Implementation
STDMETHODIMP CZipViewObject::TranslateAccelerator(LPMSG pMsg)
{
  return E_NOTIMPL;
}

STDMETHODIMP CZipViewObject::EnableModeless(BOOL fEnable)
{
  return E_NOTIMPL;
}

STDMETHODIMP CZipViewObject::UIActivate(UINT uiState)
{
  // if(uiState == SVUIA_ACTIVATE_FOCUS)
  //  onSetFocus(NULL);
  return S_OK;
}

STDMETHODIMP CZipViewObject::Refresh(void)
{
  m_ListView.DeleteAllItems();
  
  RefreshListCtrl();
  return S_OK;
}

void CZipViewObject::AddCulumnItemInfo(
    const NZipSettings::CListViewInfo &aListViewInfo,
    PROPID aPropID)
{
  if (m_RegistryListViewInfo.FindColumnWithID(aPropID) >= 0)
    return;
  int anIndex = aListViewInfo.FindColumnWithID(aPropID);
  if (anIndex >= 0)
    m_RegistryListViewInfo.ColumnInfoVector.push_back(
        aListViewInfo.ColumnInfoVector[anIndex]);
  else
  {
    NZipSettings::CColumnInfo aColumnInfo;
    aColumnInfo.IsVisible = true;
    aColumnInfo.PropID = aPropID;
    aColumnInfo.Width = kDefaultColumnWidth;
    m_RegistryListViewInfo.ColumnInfoVector.push_back(aColumnInfo);
  }
}

STDMETHODIMP CZipViewObject::CreateViewWindow(LPSHELLVIEW aPrevView, 
    LPCFOLDERSETTINGS aFolderSetings, 
    LPSHELLBROWSER aShellBrowser, 
    LPRECT aWindowRect, 
    HWND *aCreatedWindow)
{
  NZipSettings::CListViewInfo aListViewInfo;
  NZipSettings::CColumnInfoVector &aColumnInfoList = aListViewInfo.ColumnInfoVector;
  if(aPrevView != NULL)
  {
    CComPtr<IColumnInfoTransfer> aColumnInfoTransfer;
    if(aPrevView->QueryInterface(IID_IColumnInfoTransfer, (void **)&aColumnInfoTransfer) == S_OK)
      aColumnInfoTransfer->GetListViewInfo(aListViewInfo);
    else
      m_ProxyHandler->ReadColumnsInfo(aListViewInfo);
  }
  else
    m_ProxyHandler->ReadColumnsInfo(aListViewInfo);
  if(aListViewInfo.SortIndex < 0 || 
      aListViewInfo.SortIndex >= aColumnInfoList.size())
    aListViewInfo.SortIndex = 0;

  m_RegistryListViewInfo.SortIndex = aListViewInfo.SortIndex;
  m_RegistryListViewInfo.Ascending = aListViewInfo.Ascending;
  m_RegistryListViewInfo.ColumnInfoVector.clear();

  for(int i = 0; i < aListViewInfo.ColumnInfoVector.size(); i++)
  {
    const NZipSettings::CColumnInfo &aColumnInfo = aListViewInfo.ColumnInfoVector[i];
    if(!aColumnInfo.IsVisible)
      continue;
    if(aColumnInfo.PropID == kaipidType)
    {
      AddCulumnItemInfo(aListViewInfo, kaipidType);
      continue;
    }
    // m_ProxyHandler->ReadProperties(m_ZipFolder->m_ArchiveHandler);
    for(int j = 0; j < m_ProxyHandler->m_ColumnsProperties.Size(); j++)
      if(aColumnInfo.PropID == m_ProxyHandler->m_ColumnsProperties[j].ID)
      {
        AddCulumnItemInfo(aListViewInfo, aColumnInfo.PropID);
        break;
      }
  }

  for(i = 0; i < m_ProxyHandler->m_ColumnsProperties.Size(); i++)
  {
    const CArchiveItemProperty &aProperty = m_ProxyHandler->m_ColumnsProperties[i];
    AddCulumnItemInfo(aListViewInfo, aProperty.ID);
  }
  AddCulumnItemInfo(aListViewInfo, kaipidType);

  m_RegistryListViewInfo.OrderItems();
  
  // MessageBox(NULL, "CZipViewObject::CreateViewWindow", "", MB_OK);
  WNDCLASS wc = { 0 };
  
  *aCreatedWindow = NULL;
  
  // Register the class once
  if(!GetClassInfo(_Module.m_hInst, NAMESPACEVIEW_CLASS, &wc))
  {
    wc.style          = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc    = (WNDPROC) NameSpaceWndProc;
    wc.cbClsExtra     = NULL;
    wc.cbWndExtra     = NULL;
    wc.hInstance      = _Module.m_hInst;
    wc.hIcon          = NULL;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = NAMESPACEVIEW_CLASS;
    
    if (RegisterClass(&wc) == 0)
      return E_FAIL;
  }
  
  // Store the browser pointer
  m_ShellBrowser = aShellBrowser;
  m_ShellBrowser->AddRef();

  int aPartsSizes[] = {240, -1};
  LRESULT lResult;
  m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETPARTS, 2, 
    (LPARAM)aPartsSizes, &lResult);

  
  m_FolderSettings = *aFolderSetings;
  
  m_ShellBrowser->GetWindow(&m_hWndParent);

  *aCreatedWindow = CreateWindowEx(0, NAMESPACEVIEW_CLASS,
      NULL,
      WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      aWindowRect->left, aWindowRect->top,
      aWindowRect->right - aWindowRect->left,
      aWindowRect->bottom - aWindowRect->top,
      m_hWndParent,
      NULL,
      _Module.m_hInst,
      (LPVOID) this);
  
  if(NULL == *aCreatedWindow)
    return E_FAIL;

  RestoreViewState(); 
  return S_OK;
}

STDMETHODIMP CZipViewObject::DestroyViewWindow(void)
{
  NZipSettings::CListViewInfo aRegistryListViewInfo;
  GetListViewInfo(aRegistryListViewInfo);
  m_ProxyHandler->SaveColumnsInfo(aRegistryListViewInfo);
  m_ListView.Destroy();
  UIActivate(SVUIA_DEACTIVATE);
  m_Window.Destroy();
  m_ShellBrowser->Release();
  
  return S_OK;
}

STDMETHODIMP CZipViewObject::GetCurrentInfo(LPFOLDERSETTINGS aFolderSetings)
{
  *aFolderSetings = m_FolderSettings;
  return S_OK;
}

STDMETHODIMP CZipViewObject::AddPropertySheetPages( DWORD dwReserved, 
    LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
  return E_NOTIMPL;
}

STDMETHODIMP CZipViewObject::SaveViewState(void)
{
  return E_NOTIMPL;
}

STDMETHODIMP CZipViewObject::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
  return E_NOTIMPL;
}

STDMETHODIMP CZipViewObject::GetItemObject(UINT uItem, REFIID riid, 
    LPVOID *ppvOut)
{
  *ppvOut = NULL;
  return E_NOTIMPL;
}

struct CInternalColumnInfo
{
  PROPID PropID;
  // bool IsVisible;
  UINT32 Order;
  UINT32 Width;
};

static bool operator <(const CInternalColumnInfo &a1, const CInternalColumnInfo &a2)
  {  return a1.Order < a2.Order; }

/////////////////////////////////////////////////
// IColumnInfoTransfer

STDMETHODIMP CZipViewObject::GetListViewInfo(NZipSettings::CListViewInfo &aListViewInfo)
{
  NZipSettings::CColumnInfoVector &aColumnInfoList = aListViewInfo.ColumnInfoVector;
  aColumnInfoList.clear();
  std::vector<CInternalColumnInfo> aVector;
  for(int i = 0; i < m_ColumnsPropIDs.Size(); i++)
  {
    CInternalColumnInfo aColumnInfo;
    LVCOLUMN aWinColumnInfo;
    aWinColumnInfo.mask = LVCF_ORDER | LVCF_WIDTH;
    bool aResult = m_ListView.GetColumn(i, &aWinColumnInfo);
    aColumnInfo.Order = aWinColumnInfo.iOrder;
    aColumnInfo.Width = aWinColumnInfo.cx;
    aColumnInfo.PropID = m_ColumnsPropIDs[i];
    aVector.push_back(aColumnInfo);
  }
  std::sort(aVector.begin(), aVector.end());
  for(i = 0; i < aVector.size(); i++)
  {
    const CInternalColumnInfo &anInternalColumnInfo = aVector[i];
    NZipSettings::CColumnInfo aColumnInfo;
    aColumnInfo.IsVisible = true;
    aColumnInfo.Width = anInternalColumnInfo.Width;
    aColumnInfo.PropID = anInternalColumnInfo.PropID;
    aColumnInfoList.push_back(aColumnInfo);
  }
  for(;i < m_RegistryListViewInfo.ColumnInfoVector.size(); i++)
  {
    const NZipSettings::CColumnInfo &aColumnInfo = m_RegistryListViewInfo.ColumnInfoVector[i];
    aColumnInfoList.push_back(aColumnInfo);
  }

  aListViewInfo.SortIndex = m_RegistryListViewInfo.SortIndex;
  aListViewInfo.Ascending = m_RegistryListViewInfo.Ascending;
  return S_OK;
}


LRESULT CALLBACK CZipViewObject::NameSpaceWndProc(HWND hWnd,
    UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  CWindow aWindow(hWnd);
  CZipViewObject  *pThis = (CZipViewObject*) aWindow.GetLongPtr(GWL_USERDATA);
  switch (uiMsg)
  {
  case WM_NCCREATE:
    {
      LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
      pThis = (CZipViewObject*) (lpcs->lpCreateParams);
      aWindow.SetLongPtr(GWL_USERDATA, (LONG_PTR)pThis);
      pThis->m_Window = aWindow;
      
      //set the window handle
    }
    break;
  case WM_CREATE:
    return pThis->onCreate();
  case WM_ERASEBKGND:
    return pThis->onEraseBkGnd((HDC) wParam);
  case WM_SIZE:
    return pThis->onSize(LOWORD(lParam), HIWORD(lParam));
  case WM_NOTIFY:
    return pThis->onNotify((UINT)wParam, (LPNMHDR)lParam);
  case WM_COMMAND:
    return pThis->onCommand((HWND) lParam, HIWORD(wParam), LOWORD(wParam));
  case WM_SETFOCUS:
    return pThis->onSetFocus((HWND) wParam);
  case WM_USER_UPDATESTATUS:
    pThis->ShowStatusBar();
    return 0;
  case WM_CONTEXTMENU:
    if (pThis->OnContextMenu(HANDLE(wParam), GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
      return 0;
  }
  return DefWindowProc(hWnd, uiMsg, wParam, lParam);
}

bool CZipViewObject::OnContextMenu(HANDLE aWindowHandle, int xPos, int yPos)
{
  if (aWindowHandle != m_ListView)
    return false;

  int aNumSelectedItems = m_ListView.GetSelectedCount();

  if (xPos < 0 || yPos < 0)
  {
    if (aNumSelectedItems == 0)
    {
      xPos = 0;
      yPos = 0;
    }
    else
    {
      int anItemIndex = m_ListView.GetNextItem(-1, LVNI_FOCUSED);
      if (anItemIndex == -1)
        return false;
      RECT aRect;
      if (!m_ListView.GetItemRect(anItemIndex, &aRect, LVIR_ICON))
        return false;
      xPos = (aRect.left + aRect.right) / 2;
      yPos = (aRect.top + aRect.bottom) / 2;
    }
    POINT aPoint = {xPos, yPos };
    m_ListView.ClientToScreen(&aPoint);
    xPos = aPoint.x;
    yPos = aPoint.y;
  }

  CMenu aMenu;
  aMenu.CreatePopup();

  if (aNumSelectedItems > 0)
    aMenu.AppendItem(MF_STRING, IDC_MY_CONTEXT_MENU_OPEN, 
        LangLoadString(IDS_LISTVIEW_CONTEXT_MENU_OPEN, 0x02000411));
  aMenu.AppendItem(MF_STRING, IDC_MY_CONTEXT_MENU_EXTRACT, 
        LangLoadString(IDS_LISTVIEW_CONTEXT_MENU_EXTRACT, 0x02000412));
    
  int aResult = aMenu.Track(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, 
    xPos, yPos, m_ListView);
  switch(aResult)
  {
    case IDC_MY_CONTEXT_MENU_OPEN:
      OpenSelectedItems();
      break;
    case IDC_MY_CONTEXT_MENU_EXTRACT:
      CommandExtract();
      break;
  }
  return true;
}

///////////////////////////////////////////////////////////
// Message Handlers
long CZipViewObject::onEraseBkGnd(HDC hDC)
{
  return 1L;
}

LRESULT CZipViewObject::onSize(WORD nCx, WORD nCy)
{
  if(m_ListView.IsWindow())
    m_ListView.MoveWindow(0, 0, nCx, nCy, false);
  return 0;
}

/*
int FindStringInList(const UStringVector &aList, const UString &aString)
{
  for(int i = 0; i < aList.Size(); i++)
    if(aString == aList[i])
      return i;
  return -1;
}
*/

void CZipViewObject::GetSelectedItemsIndexes(CRecordVector<UINT32> &anIndexes)
{
  anIndexes.Clear();
  int anItemIndex = -1;
  while ((anItemIndex = m_ListView.GetNextItem(anItemIndex, LVNI_SELECTED)) != -1)
  {
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = anItemIndex;
    m_ListView.GetItem(&lvi);
    anIndexes.Add(lvi.lParam);
  }
}

void CZipViewObject::OpenFolder(UINT32 anIndex)
{
  // if(!aDirOrFileIndex.IsDirIndex())
  //   throw 1;
  /*
  CShellItemIDList aRealativeIDList(m_IDListManager);
  CreateIDListFromIndex(aRealativeIDList, anIndex);
  m_ShellBrowser->BrowseObject(aRealativeIDList, 
      SBSP_SAMEBROWSER | SBSP_RELATIVE);
  */

  CShellItemIDList aNewAbsoluteIDPathSpec(m_IDListManager);
  CShellItemIDList aRealativeIDList(m_IDListManager);
  CreateIDListFromIndex(aRealativeIDList, anIndex);
  LPITEMIDLIST aNewAbsoluteIDPath = m_IDListManager->Concatenate(
      m_ZipFolder->m_AbsoluteIDList, 
      aRealativeIDList);
  aNewAbsoluteIDPathSpec.Attach(aNewAbsoluteIDPath);
  m_ShellBrowser->BrowseObject(aNewAbsoluteIDPath, 
    SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
  return;
}

void CZipViewObject::OpenSelectedItems()
{
  CRecordVector<UINT32> anIndexes;
  GetSelectedItemsIndexes(anIndexes);
  if (anIndexes.Size() > 30)
  {
    MyMessageBox(IDS_ERROR_TOO_MUCH_ITEMS, 0x02000606);
    return;
  }
  bool aDirIsStarted = false;
  for(int i = 0; i < anIndexes.Size(); i++)
  {
    UINT32 anIndex = anIndexes[i];
    if (IsObjectFolder(m_ArchiveFolder, anIndex))
    {
      if (!aDirIsStarted)
      {
        OpenFolder(anIndex);
        aDirIsStarted = true;
      }
    }
    else
      OpenItem(anIndex);
  }
}


static LPCTSTR kHelpTopic = _T("gui/main.htm");

void CZipViewObject::CommandHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
}


LRESULT CZipViewObject::onKeyDown(LPNMLVKEYDOWN aKeyDownInfo)
{
  switch(aKeyDownInfo->wVKey)
  {
    /*
    case VK_DELETE:
      CommandDelete();
      return 0;
    */
    case VK_F1:
      CommandHelp();
      return 0;
    case 'A':
      if((GetKeyState(VK_CONTROL) & 0x8000) != 0)
      {
        m_RedrawEnabled = false;
        m_ListView.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
        m_RedrawEnabled = true;
        // ShowStatusBar();
      }
      return 0;
  }
  return 0;
}

CSysString CZipViewObject::GetFileType(UINT32 anIndex)
{
  bool anIsDir = IsObjectFolder(m_ArchiveFolder, anIndex);
  if(anIsDir)
  {
    if(m_DirFileTypeStringDefined)
      return m_DirFileTypeString;
  }
  // LPCITEMIDLIST aProperties = GetPropertiesOfItem(aDirOrFileIndex);
  UString anExtension;
  if(!anIsDir)
  {
    anExtension = GetExtensionOfObject(m_ArchiveFolder, anIndex);
    CUStringToCSysString::const_iterator anIterator = 
        m_ExtensionToFileTypeMap.find(anExtension);
    if(anIterator != m_ExtensionToFileTypeMap.end())
      return(*anIterator).second;
  }

  DWORD anAttributes = FILE_ATTRIBUTE_NORMAL;
  if(anIsDir)
    anAttributes |= FILE_ATTRIBUTE_DIRECTORY;

  SHFILEINFO aFileInfo;
  CSysString aFileType = 
  ::SHGetFileInfo(GetSystemString(
      GetNameOfObject(m_ArchiveFolder, anIndex)), 
      anAttributes,  &aFileInfo, sizeof(aFileInfo), 
      SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME);

  if(anIsDir)
  {
    m_DirFileTypeStringDefined = true;
    m_DirFileTypeString = aFileInfo.szTypeName;
  }
  else
    m_ExtensionToFileTypeMap.insert(
        CUStringToCSysString::value_type(anExtension, aFileInfo.szTypeName));
  return aFileInfo.szTypeName;
}


LRESULT CZipViewObject::SetItemText(LV_DISPINFO *aDispInfo, UINT32 anIndex)
{
  CSysString aString;
  UINT32 aSubItemIndex = aDispInfo->item.iSubItem;
  {
    NCOM::CPropVariant aProperty;
    PROPID aPropID = m_ColumnsPropIDs[aSubItemIndex];
    if(aPropID == kaipidType)
      aString = GetFileType(anIndex);
    else
    {
      HRESULT aResult = m_ArchiveFolder->GetProperty(anIndex, aPropID, &aProperty);
      if (aResult != S_OK)
      {
        // PrintMessage("GetPropertyValue error");
        return 0;
      }
      aString = ConvertPropertyToString(aProperty, aPropID, false);
    }
  }
  
  int aSize = aDispInfo->item.cchTextMax;
  if(aSize > 0)
  {
    if(aString.Length() + 1 > aSize)
      aString = aString.Left(aSize - 1);
    lstrcpy(aDispInfo->item.pszText, aString);
  }
  /*
  else
    PrintMessage("LVN_GETDISPINFO error");
  */
  return 0;
}

LRESULT CZipViewObject::SetIcon(LV_DISPINFO *aDispInfo, UINT32 anIndex)
{
  // LPCITEMIDLIST aProperties = GetPropertiesOfItem(aDirOrFileIndex);
  UString anExtension;
  if(IsObjectFolder(m_ArchiveFolder, anIndex))
  {
    if(m_DirIconIndex >= 0)
    {
      aDispInfo->item.iImage = m_DirIconIndex;
      return 0;
    }
  }
  else // Is File Index
  {
    anExtension = GetExtensionOfObject(m_ArchiveFolder, anIndex);
    anExtension.MakeUpper();
    CUStringToIntMap::const_iterator anIterator = m_ExtensionToIconIndexMap.find(anExtension);
    if(anIterator != m_ExtensionToIconIndexMap.end())
    {
      aDispInfo->item.iImage = (*anIterator).second;
      return 0;
    }
  }
  CComPtr<IExtractIcon> anExtractIcon;
  CShellItemIDList aShellItemIDList(m_IDListManager);
  CreateIDListFromIndex(aShellItemIDList, anIndex);
  LPCITEMIDLIST apidl = aShellItemIDList;
  if(SUCCEEDED(m_ShellFolder->GetUIObjectOf(m_Window, 1, 
      &apidl, IID_IExtractIcon, NULL, (LPVOID*)&anExtractIcon)))
  {
    TCHAR aName[MAX_PATH];
    UINT anOutFlags = 0;
    INT anOutIndex;
    anExtractIcon->GetIconLocation(GIL_FORSHELL, aName, 
      MAX_PATH, &anOutIndex, &anOutFlags);
    if((anOutFlags & GIL_NOTFILENAME) != 0)
    {
      HICON aLargeIcon = NULL;
      HICON aSmallIcon = NULL;
      int cxIcon = GetSystemMetrics(SM_CXICON);
      int cxSmIcon = GetSystemMetrics(SM_CXSMICON);
      HRESULT aResult = anExtractIcon->Extract(aName, anOutIndex,
        &aLargeIcon, &aSmallIcon, MAKELONG(cxIcon, cxSmIcon));
      if (aResult == S_OK)
      {
        m_LargeImageList.Add(aLargeIcon); // indexes must be same
        int anIconIndex = m_SmallImageList.Add(aSmallIcon);
        if(IsObjectFolder(m_ArchiveFolder, anIndex))
          m_DirIconIndex = anIconIndex;
        else
          m_ExtensionToIconIndexMap.insert(CUStringToIntMap::value_type(anExtension, anIconIndex));
        aDispInfo->item.iImage = anIconIndex;
      }
    }
  }
  return 0;
}


LRESULT CZipViewObject::onColumnClick(LPNMLISTVIEW aListView)
{
  int anIndex = aListView->iSubItem;
  int aNewSortIndex = m_RegistryListViewInfo.FindColumnWithID(m_ColumnsPropIDs[anIndex]);
  if(aNewSortIndex == m_RegistryListViewInfo.SortIndex)
    m_RegistryListViewInfo.Ascending = !m_RegistryListViewInfo.Ascending;
  else
  {
    m_RegistryListViewInfo.Ascending = true;
    m_RegistryListViewInfo.SortIndex = aNewSortIndex;
  }

  // Sort the items...
  m_ListView.SortItems(CompareItems, (LPARAM)this);
  return 0;
}

static bool ColumnLessFunc(const NColumnsDialog::CColumnInfo &a1, 
    const NColumnsDialog::CColumnInfo &a2)
{
  if(a1.IsVisible && !a2.IsVisible)
    return true;
  if(!a1.IsVisible && a2.IsVisible)
    return false;
  return a1.Order < a2.Order;
}

void CZipViewObject::onColumnCommand()
{
  CColumnsDialog aDialog;
  NZipSettings::CListViewInfo aRegistryListViewInfo;
  GetListViewInfo(aRegistryListViewInfo);
  m_RegistryListViewInfo = aRegistryListViewInfo;

  for(int i = 0; i < m_RegistryListViewInfo.ColumnInfoVector.size(); i++)
  {
    const NZipSettings::CColumnInfo &aRegColumnInfo = 
        m_RegistryListViewInfo.ColumnInfoVector[i];
    NColumnsDialog::CColumnInfo aDialogColumnInfo;
   
    aDialogColumnInfo.IsVisible = aRegColumnInfo.IsVisible;
    aDialogColumnInfo.Order = i;
    aDialogColumnInfo.Width = aRegColumnInfo.Width;
    aDialogColumnInfo.PropID = aRegColumnInfo.PropID;
    aDialogColumnInfo.Caption = m_ProxyHandler->GetNameOfProperty(aRegColumnInfo.PropID);
    aDialog.m_ColumnInfoVector.push_back(aDialogColumnInfo);
  }

  PROPID aSortPropID = 
      m_RegistryListViewInfo.ColumnInfoVector[m_RegistryListViewInfo.SortIndex].PropID;

  if(aDialog.Create(m_Window) != IDOK)
    return;

  for(i = 0; i < aDialog.m_ColumnInfoVector.size(); i++)
  {
    NColumnsDialog::CColumnInfo &aDialogColumnInfo = aDialog.m_ColumnInfoVector[i];
    if(aDialogColumnInfo.PropID == kaipidName)
      aDialogColumnInfo.IsVisible = true;
  }
  std::sort(aDialog.m_ColumnInfoVector.begin(), 
      aDialog.m_ColumnInfoVector.end(), ColumnLessFunc);

  m_RegistryListViewInfo.SortIndex = -1;
  m_RegistryListViewInfo.ColumnInfoVector.clear();
  for(i = 0; i < aDialog.m_ColumnInfoVector.size(); i++)
  {
    NZipSettings::CColumnInfo aRegColumnInfo;
    const NColumnsDialog::CColumnInfo &aDialogColumnInfo = 
        aDialog.m_ColumnInfoVector[i];
    aRegColumnInfo.PropID = aDialogColumnInfo.PropID;
    aRegColumnInfo.IsVisible = aDialogColumnInfo.IsVisible;
    if(aSortPropID == aDialogColumnInfo.PropID)
      m_RegistryListViewInfo.SortIndex = i;
    aRegColumnInfo.Width = aDialogColumnInfo.Width;
    m_RegistryListViewInfo.ColumnInfoVector.push_back(aRegColumnInfo);
  }
  m_RegistryListViewInfo.OrderItems();
  if(m_RegistryListViewInfo.SortIndex < 0)
  {
    m_RegistryListViewInfo.SortIndex = m_RegistryListViewInfo.FindColumnWithID(kaipidName);
    m_RegistryListViewInfo.Ascending = true;
  }

  InitListCtrl();
  RefreshListCtrl();
}

// 4.71
LRESULT CZipViewObject::onRightClick(LPNMITEMACTIVATE anItemActiveate)
{
  if(anItemActiveate->hdr.hwndFrom == HWND(m_ListView))
    return 0;

  POINT aPoint;
  ::GetCursorPos(&aPoint);

  CMenu aMenu;
  aMenu.CreatePopup();

  aMenu.AppendItem(MF_STRING, IDC_MY_CONTEXT_MENU_COLUMN, 
      LangLoadString(IDS_LISTVIEW_COLUMNS_CONTEXT_MENU, 0x02000401));
  
  int aResult = aMenu.Track(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, 
      aPoint.x, aPoint.y, m_ListView);
  if(aResult == IDC_MY_CONTEXT_MENU_COLUMN)
    onColumnCommand();
  return TRUE;
}

LRESULT CZipViewObject::onNotify(UINT CtlID, LPNMHDR lpnmh)
{
  LRESULT aResult;
  switch(lpnmh->code)
  {
  case LVN_GETDISPINFO:
    {
      // TODO : Set Display Info
      LV_DISPINFO  *aDispInfo = (LV_DISPINFO *)lpnmh;

      //is the sub-item information being requested?
      UINT32 anIndex = aDispInfo->item.lParam;

      //const CArchiveFolderItem &anItem = (*m_ArchiveFolder->m_SubItems)[anIndex];
      if((aDispInfo->item.mask & LVIF_TEXT) != 0)
        return SetItemText(aDispInfo, anIndex);
      if(aDispInfo->item.mask & LVIF_IMAGE)
        return SetIcon(aDispInfo, anIndex);
      return 0;
    }
  case LVN_KEYDOWN:
    aResult = onKeyDown(LPNMLVKEYDOWN(lpnmh));
    SendRefreshStatusBarMessage();
    return aResult;
  case LVN_COLUMNCLICK:
    return onColumnClick(LPNMLISTVIEW(lpnmh));
  case NM_DBLCLK:
    SendRefreshStatusBarMessage();
    OpenSelectedItems();
    return 0;
  case NM_CLICK:
    SendRefreshStatusBarMessage();
    return 0;
  case NM_RETURN:
    OpenSelectedItems();

    // TODO : Handler default action...
    return 0;
  case NM_RCLICK:
    aResult = onRightClick((LPNMITEMACTIVATE)lpnmh);
    SendRefreshStatusBarMessage();
    return aResult;
  case LVN_BEGINDRAG:
  case LVN_BEGINRDRAG:
    {
      SendRefreshStatusBarMessage();
      return 0;
    }
  case LVN_ITEMCHANGED:
    {
      NMLISTVIEW *pNMLV = (NMLISTVIEW *) lpnmh;
      SelChange(pNMLV);
      return TRUE;
    }
  case NM_SETFOCUS:
    return onSetFocus(NULL);
  case NM_KILLFOCUS:
    return onKillFocus(NULL);
  }
  return 0;
}

void CZipViewObject::SendRefreshStatusBarMessage()
{
  m_Window.PostMessage(WM_USER_UPDATESTATUS);
}

void CZipViewObject::ShowStatusBar()
{
  if(!m_RedrawEnabled)
    return;
  CSysString aString;
  int aNumSelectedItems = m_ListView.GetSelectedCount();
  LRESULT lResult;
  if(aNumSelectedItems == 0)
  {
    UINT32 aNumItems;
    m_ArchiveFolder->GetNumberOfItems(&aNumItems);
    aString = MyFormat(m_NObjectsFormat, NumberToString(aNumItems));
  }
  else
    aString = MyFormat(m_NSelectedItemsFormat, NumberToString(aNumSelectedItems));
  
  m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 0, 
      (LPARAM)(LPCTSTR)aString, &lResult);

  if(aNumSelectedItems == 0)
  {
    aString = MyFormat(m_FileSizeFormat, NumberToString(m_DirectorySize));
    m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 1, 
        (LPARAM)(LPCTSTR)aString, &lResult);
  }
  else
  {
    if(m_NumSelectedFiles == 0)
      m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 1, 
      (LPARAM)_T(""), &lResult);
    else
    {
      aString = MyFormat(m_FileSizeFormat, NumberToString(m_SelectedItemsSize));
      m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 1, 
        (LPARAM)(LPCTSTR)aString, &lResult);
    }
  }
}

void CZipViewObject::SelChange(NMLISTVIEW *pNMLV)
{
  UINT32 anIndex = pNMLV->lParam;
  bool anOldSelected = (pNMLV->uOldState & LVIS_SELECTED) != 0;
  bool aNewSelected = (pNMLV->uNewState & LVIS_SELECTED) != 0;
  if(!anOldSelected == aNewSelected)
  {
    if (IsObjectFolder(m_ArchiveFolder, anIndex))
    {
      // ShowStatusBar();
      return;
    }
    NCOM::CPropVariant aProperty;
    HRESULT aResult = m_ArchiveFolder->GetProperty(anIndex, kaipidSize, &aProperty);
    if (aResult == S_OK)
    {
      UINT64 aSize = (aProperty.vt == VT_EMPTY) ? 0:
          ConvertPropVariantToUINT64(aProperty);
      if(aNewSelected)
      {
        m_SelectedItemsSize += aSize;
        m_NumSelectedFiles++;
      }
      else
      {
        m_SelectedItemsSize -= aSize;
        m_NumSelectedFiles--;
      }
      // ShowStatusBar();
    }
    return;
  }
}

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

struct CViewTypeTuple
{
  UINT FolderViewMode;
  DWORD ListViewStyle;
  UINT BitmapIndex;
  UINT CommandID;
};

static CViewTypeTuple kViewTypeTuple[] = 
{
  { FVM_ICON, LVS_ICON, VIEW_LARGEICONS, IDC_VIEW_ICON },
  { FVM_SMALLICON, LVS_SMALLICON, VIEW_SMALLICONS, IDC_VIEW_SMALLICON },
  { FVM_LIST, LVS_LIST, VIEW_LIST, IDC_VIEW_LIST },
  { FVM_DETAILS, LVS_REPORT, VIEW_DETAILS, IDC_VIEW_DETAILS}
};

static const kNumViewTypes = ARRAYSIZE(kViewTypeTuple);

int GetFolderViewModeIndex(UINT aFolderViewMode)
{
  for (int i = 0; i < kNumViewTypes; i++)
    if (aFolderViewMode == kViewTypeTuple[i].FolderViewMode)
      return i;
  return -1;
}

int GetListViewStyleIndex(DWORD aListViewStyle)
{
  for (int i = 0; i < kNumViewTypes; i++)
    if (aListViewStyle == kViewTypeTuple[i].ListViewStyle)
      return i;
  return -1;
}

int GetBitmapIndexIndex(DWORD aBitmapIndex)
{
  for (int i = 0; i < kNumViewTypes; i++)
    if (aBitmapIndex == kViewTypeTuple[i].BitmapIndex)
      return i;
  return -1;
}

int GetCommandIDIndex(DWORD aCommandID)
{
  for (int i = 0; i < kNumViewTypes; i++)
    if (aCommandID == kViewTypeTuple[i].CommandID)
      return i;
  return -1;
}

#define ID_LISTVIEW   1001

static const DWORD kListViewStyles = 
  WS_CHILD |WS_BORDER | WS_VISIBLE | LVS_SHAREIMAGELISTS; //  | LVS_SHOWSELALWAYS;

LRESULT CZipViewObject::onCreate(void)
{
  DWORD dwStyle = kListViewStyles;

  int anIndex = GetFolderViewModeIndex(m_FolderSettings.ViewMode);
  if(anIndex == -1)
  {
    m_FolderSettings.ViewMode = FVM_DETAILS;
    anIndex = GetFolderViewModeIndex(m_FolderSettings.ViewMode);
  }
  dwStyle |= kViewTypeTuple[anIndex].ListViewStyle;

  dwStyle |= LVS_AUTOARRANGE;

  // My
  dwStyle |=   WS_TABSTOP | WS_VISIBLE | WS_CHILD | 
            WS_BORDER | 
            LVS_SHAREIMAGELISTS;

  DWORD anExStyle;
  anExStyle = WS_EX_CLIENTEDGE;
  
  m_ListView.CreateEx(anExStyle, dwStyle, 0, 0, 0, 0, 
      m_Window, (HMENU)ID_LISTVIEW, _Module.m_hInst, NULL);

  m_ListView.SetImageList(m_LargeImageList, LVSIL_NORMAL);
  m_ListView.SetImageList(m_SmallImageList, LVSIL_SMALL);

  DWORD anExtendedStyle = m_ListView.GetExtendedListViewStyle();
  anExtendedStyle |= LVS_EX_HEADERDRAGDROP; // Version 4.70
  m_ListView.SetExtendedListViewStyle(anExtendedStyle);

  m_ListView.ShowWindow(SW_SHOW);
  m_ListView.InvalidateRect(NULL, true);
  m_ListView.UpdateWindow();
  
  InitListCtrl();
  RefreshListCtrl();
  
  // TODO : Add code to create a window / list view control here
  return S_OK;
}

static int GetColumnAlign(PROPID aPropID, VARTYPE aVarType)
{
  switch(aPropID)
  {
    case kaipidCreationTime:
    case kaipidLastAccessTime:
    case kaipidLastWriteTime:
      return LVCFMT_LEFT;
  }
  switch(aVarType)
  {
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_INT:
    case VT_UI4:
    case VT_UINT:
    case VT_I8:
    case VT_UI8:
    case VT_BOOL:    
      return LVCFMT_RIGHT;
    
    case VT_EMPTY:
    case VT_I1:
    case VT_FILETIME:
    case VT_BSTR:
      return LVCFMT_LEFT;
    
    default:
      return LVCFMT_CENTER;
  }
}

void CZipViewObject::InitListCtrl()
{
  m_ListView.DeleteAllItems();
  for(int i = 0; i < m_ColumnsPropIDs.Size(); i++)
    m_ListView.DeleteColumn(0);

  m_ColumnsPropIDs.Clear();

  LV_COLUMN   lvColumn;
  TCHAR       szString[1024];
  lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_ORDER;

  int anItemIndex = 0;

  int aNameIndex = -1;
  for(i = 0; i < m_RegistryListViewInfo.ColumnInfoVector.size(); i++)
    if(m_RegistryListViewInfo.ColumnInfoVector[i].PropID == kaipidName)
    {
      aNameIndex = i;
      break;
    }
  if (aNameIndex < 0)
    return; // error;

  for(i = 0; i < m_RegistryListViewInfo.ColumnInfoVector.size(); i++)
  {
    int anUsedIndex;
    if(i == 0)
      anUsedIndex = aNameIndex;
    else if(i <= aNameIndex)
      anUsedIndex = i - 1;
    else
      anUsedIndex = i;


    NZipSettings::CColumnInfo aColumnInfo = 
        m_RegistryListViewInfo.ColumnInfoVector[anUsedIndex];
    if(!aColumnInfo.IsVisible)
      continue;

    lvColumn.cx = aColumnInfo.Width;

    lvColumn.iOrder = anUsedIndex;

    lvColumn.fmt = GetColumnAlign(aColumnInfo.PropID, 
        m_ProxyHandler->GetTypeOfProperty(aColumnInfo.PropID));
    
    lvColumn.pszText = szString;
    lstrcpy(szString, m_ProxyHandler->GetNameOfProperty(aColumnInfo.PropID));

    lvColumn.iSubItem = anItemIndex++;

    m_ColumnsPropIDs.Add(aColumnInfo.PropID);
    m_ListView.InsertColumn(anItemIndex, &lvColumn);
  }
}

void CZipViewObject::RefreshListCtrl()
{
  m_SelectedItemsSize = 0;
  m_NumSelectedFiles = 0;

  /*
  UINT64 aStart, anEnd;
  NTimer::QueryPerformanceCounter(aStart);
  */

  if (!m_ListView.IsWindow())
    return;
  m_ListView.SetRedraw(false);
  m_RedrawEnabled = false;

  LVITEM lvi;
  ZeroMemory(&lvi, sizeof(lvi));
  lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
  lvi.iSubItem = 0;
  
  int anItemIndex = 0;


  UINT32 aNumItems;
  m_ArchiveFolder->GetNumberOfItems(&aNumItems);

  m_DirectorySize = 0;
  for(UINT32 i = 0; i < aNumItems; i++)
  {
    lvi.iItem = i;
    
    TCHAR aName[MAX_PATH];
    lstrcpy(aName, GetSystemString(GetNameOfObject(m_ArchiveFolder, i)));
    lvi.pszText = aName;

    lvi.lParam = i;
    lvi.iImage = I_IMAGECALLBACK;

    if (!IsObjectFolder(m_ArchiveFolder, i))
    {
      NCOM::CPropVariant aProperty;
      HRESULT aResult = m_ArchiveFolder->GetProperty(i, kaipidSize, &aProperty);
      if (aResult == S_OK)
      {
        m_DirectorySize += (aProperty.vt == VT_EMPTY) ? 0:
            ConvertPropVariantToUINT64(aProperty);
      }
    }

    if(m_ListView.InsertItem(&lvi) == -1)
    {
      // PrintMessage("InsertItem Error");
    }
  }

  // Sort the items...
  m_ListView.SortItems(CompareItems, (LPARAM)this);

  m_ListView.SetItemState(0, LVIS_FOCUSED, LVIS_FOCUSED);
  m_RedrawEnabled = true;
  m_ListView.SetRedraw(true);
  m_ListView.InvalidateRect(NULL, true);
  m_ListView.UpdateWindow();
}

int CALLBACK CZipViewObject::CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
  int aResult = CompareItems2(lParam1, lParam2, lpData);
  return ((CZipViewObject*)lpData)->m_RegistryListViewInfo.Ascending ? 
      aResult: (-aResult);
}

int CALLBACK CZipViewObject::CompareItems2(LPARAM lParam1, LPARAM lParam2, LPARAM lpData)
{
  // return 0;
  // Let the code in shellfolder do the comparing...
  if(lpData == NULL)
  {
    return 0;
  }
  CZipViewObject *aZipViewObject = (CZipViewObject*)lpData;
  UINT32 anIndex1, anIndex2;
  anIndex1 = lParam1;
  anIndex2 = lParam2;
  bool anIs1Folder = IsObjectFolder(aZipViewObject->m_ArchiveFolder, anIndex1);
  bool anIs2Folder = IsObjectFolder(aZipViewObject->m_ArchiveFolder, anIndex2);
  if(anIs1Folder && (!anIs2Folder))
    return -1;
  if((!anIs1Folder) && anIs2Folder)
    return 1;

  PROPID aPropID = aZipViewObject->m_RegistryListViewInfo.ColumnInfoVector[
      aZipViewObject->m_RegistryListViewInfo.SortIndex].PropID;

  if(aPropID == kaipidType)
    return aZipViewObject->GetFileType(anIndex1).CollateNoCase(
        aZipViewObject->GetFileType(anIndex2));

  // LPCITEMIDLIST anIDList1 = aZipViewObject->GetPropertiesOfItem(anIndex1);
  // LPCITEMIDLIST anIDList2 = aZipViewObject->GetPropertiesOfItem(anIndex2);
  if(aPropID == kaipidName)
  {
    CShellItemIDList aShellItemIDList1(aZipViewObject->m_IDListManager);
    CShellItemIDList aShellItemIDList2(aZipViewObject->m_IDListManager);
    CreateIDListFromIndex(aShellItemIDList1, anIndex1);
    CreateIDListFromIndex(aShellItemIDList2, anIndex2);
    return aZipViewObject->m_ShellFolder->CompareIDs(0, aShellItemIDList1, aShellItemIDList2);
  }

  NCOM::CPropVariant aPropVariant1, aPropVariant2;
  // Name must be first property
  aZipViewObject->m_ArchiveFolder->GetProperty(anIndex1, aPropID, &aPropVariant1);
  aZipViewObject->m_ArchiveFolder->GetProperty(anIndex2, aPropID, &aPropVariant2);
  if(aPropVariant1.vt != aPropVariant2.vt)
    return 0; // It means some BUG
  return aPropVariant1.Compare(aPropVariant2);
}

LRESULT CZipViewObject::onCommand(HWND aWndCtrl, DWORD aNotifyCode, DWORD aCommandID)
{
  int anIndex = GetCommandIDIndex(aCommandID);
  // TODO : Add extra code here
  if(anIndex >= 0)
    OnViewStyle(kViewTypeTuple[anIndex].ListViewStyle);
  else
  {
    switch(aCommandID)
    {
      case IDC_EDIT_PARENTFOLDER:
        m_ShellBrowser->BrowseObject(NULL, SBSP_SAMEBROWSER | SBSP_PARENT);
        break;
      case IDC_MY_EXTRACT:
        CommandExtract();
        break;
      /*
      case IDC_EDIT_DELETE:
        CommandDelete();
        break;
      */
    }
  }
  return 0L;
}

void CZipViewObject::OnViewStyle(UINT uiStyle)
{
  DWORD dwStyle = m_ListView.GetStyle();
  dwStyle &= ~LVS_TYPEMASK;

  int anIndex = GetListViewStyleIndex(uiStyle);
  m_FolderSettings.ViewMode = kViewTypeTuple[anIndex].FolderViewMode;

  dwStyle |= uiStyle;
  LONG aResult = m_ListView.SetStyle(dwStyle);
}

LRESULT CZipViewObject::onSetFocus(HWND hWndOld)
{
  m_ListView.SetFocus();
  if (m_ShellBrowser)
    m_ShellBrowser->OnViewWindowActive((IShellView *) this);
  ShowStatusBar();
  return 0L;
}

LRESULT CZipViewObject::onKillFocus(HWND hWndOld)
{
  LRESULT lResult;
  m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 0, 
      (LPARAM)_T(""), &lResult);

  m_ShellBrowser->SendControlMsg(FCW_STATUS, SB_SETTEXT, 1, 
      (LPARAM)_T(""), &lResult);
  return 0L;
}


void CZipViewObject::RestoreViewState() 
{ 
  MergeToolBar(); 
} 

/////////////////////////////////////
// Buttons

static void AddButtonString(IShellBrowser *aShellBrowser, 
    UINT anStringID, UINT32 aLangID, int &anIndex)
{
  LRESULT aTmpIndex;
  CSysString aString = LangLoadString(anStringID, aLangID);
  int aStringLength = aString.Length();
  LPTSTR aPointer = aString.GetBuffer(aStringLength + 2);
  aPointer[aStringLength] = 0;
  aPointer[aStringLength + 1] = 0;
  aShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDSTRING, NULL, 
      (LPARAM)aPointer, &aTmpIndex); 
  aString.ReleaseBuffer();
  anIndex = aTmpIndex;
}


static bool GetShowParrentButtonMode()
{
  OSVERSIONINFO aVersionInfo;
  ZeroMemory(&aVersionInfo, sizeof(aVersionInfo));
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!GetVersionEx(&aVersionInfo)) 
    return true;
  switch (aVersionInfo.dwPlatformId)
  {
    case VER_PLATFORM_WIN32_NT:
      return false;
    case VER_PLATFORM_WIN32_WINDOWS:
      if (aVersionInfo.dwMajorVersion > 4 || 
          (aVersionInfo.dwMajorVersion == 4 && aVersionInfo.dwMinorVersion >= 90))
        return false;
      return true;
  default:
    return true;
  }
}

static bool GetButtonTextShowMode()
{
  OSVERSIONINFO aVersionInfo;
  ZeroMemory(&aVersionInfo, sizeof(aVersionInfo));
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!GetVersionEx(&aVersionInfo)) 
    return true;
  switch (aVersionInfo.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_NT:
    return (aVersionInfo.dwMajorVersion >= 5);
  case VER_PLATFORM_WIN32_WINDOWS:
    return ((aVersionInfo.dwMajorVersion > 4) || 
      ((aVersionInfo.dwMajorVersion == 4) && 
      (aVersionInfo.dwMinorVersion > 0)));
  default:
    return false;
  }
}

void CZipViewObject::MergeToolBar() 
{ 
  enum 
  { 
    IN_STD_BMP = 0x4000, 
    IN_VIEW_BMP = 0x8000, 
  }; 
  bool aShowButtonText = GetButtonTextShowMode();

  static const kFirstNotParrentButtonIndex = 2;
  static const TBBUTTON c_tbDefault[] = 
  { 
    { VIEW_PARENTFOLDER | IN_VIEW_BMP, IDC_EDIT_PARENTFOLDER, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, -1}, 
    { 0,    0,    TBSTATE_ENABLED, TBSTYLE_SEP, {0,0}, 0, -1 }, 
    // { STD_COPY | IN_STD_BMP, IDC_EDIT_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, -1},
    // { STD_DELETE | IN_STD_BMP , IDC_EDIT_DELETE, TBSTATE_ENABLED, TBSTYLE_BUTTON , {0,0}, 0L, -1 },
    { 0, IDC_MY_EXTRACT, TBSTATE_ENABLED,  TBSTYLE_BUTTON , {0,0}, 0L, 
        aShowButtonText ? 0: -1 },
    // the bitmap indexes here are relative to the view bitmap 
    { 0,    0,    TBSTATE_ENABLED, TBSTYLE_SEP, {0,0}, 0, -1 },
    { VIEW_LARGEICONS | IN_VIEW_BMP, IDC_VIEW_ICON, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_SMALLICONS | IN_VIEW_BMP, IDC_VIEW_SMALLICON, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_LIST       | IN_VIEW_BMP, IDC_VIEW_LIST, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_DETAILS    | IN_VIEW_BMP, IDC_VIEW_DETAILS, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }
    /*
    { 0,    0,    TBSTATE_ENABLED, TBSTYLE_SEP, {0,0}, 0, -1 }, 

    { VIEW_SORTTYPE | IN_VIEW_BMP, IDC_VIEW_SORTTYPE, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_SORTNAME | IN_VIEW_BMP, IDC_VIEW_SORTNAME, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_SORTDATE | IN_VIEW_BMP, IDC_VIEW_SORTDATE, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }, 
    { VIEW_SORTSIZE | IN_VIEW_BMP, IDC_VIEW_SORTSIZE, TBSTATE_ENABLED, 
        TBSTYLE_BUTTON | TBSTYLE_CHECKGROUP, {0,0}, 0L, -1 }
    
    { VIEW_VIEWMENU | IN_VIEW_BMP, IDC_VIEW_ICON, TBSTATE_ENABLED, TBSTYLE_BUTTON |
      TBSTYLE_DROPDOWN,  {0,0}, 0L, -1}
    */

  } ; 
  
  LRESULT iStdBMOffset, iViewBMOffset, aMyCommandsOffset; 

 
  TBADDBITMAP ab; 
  ab.hInst = HINST_COMMCTRL;
  ab.nID   = IDB_STD_SMALL_COLOR; // std bitmaps 
  m_ShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &iStdBMOffset); 
  
  ab.nID   = IDB_VIEW_SMALL_COLOR;// std view bitmaps 
  m_ShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &iViewBMOffset); 
  
  ab.hInst = _Module.m_hInst;
  ab.nID = IDB_EXTRACT; // std bitmaps 
  m_ShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 1, (LPARAM)&ab, &aMyCommandsOffset); 
  LRESULT aTmp;
  ab.nID = IDB_DELETE; // std bitmaps 
  m_ShellBrowser->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 1, (LPARAM)&ab, &aTmp); 

  static const kNumMaxString = 1;
  int aStringIndex[kNumMaxString];
  if (aShowButtonText)
  {
    AddButtonString(m_ShellBrowser, IDS_TOOLBAR_EXTRACT, 0x02000501, aStringIndex[0]);
  }


  TBBUTTON tbActual[ARRAYSIZE(c_tbDefault)]; 

  int aViewModeIndex = GetFolderViewModeIndex(FOLDERVIEWMODE(m_FolderSettings.ViewMode));

  int anIndexOffset = GetShowParrentButtonMode() ? 0 : kFirstNotParrentButtonIndex;
  int aNumButtons = ARRAYSIZE(c_tbDefault) - anIndexOffset;

  for (int i = 0; i < aNumButtons; i++) 
  { 
    tbActual[i] = c_tbDefault[i + anIndexOffset]; 
    if (!(tbActual[i].fsStyle & TBSTYLE_SEP)) 
    { 
      if (tbActual[i].iBitmap & IN_VIEW_BMP) 
      { 
        int aBitmapIndex = tbActual[i].iBitmap & ~IN_VIEW_BMP;
        int aCurViewModeIndex = GetBitmapIndexIndex(aBitmapIndex);
        if(aCurViewModeIndex == aViewModeIndex)
          tbActual[i].fsState |= TBSTATE_CHECKED;
        tbActual[i].iBitmap = aBitmapIndex + iViewBMOffset; 
      } 
      else if (tbActual[i].iBitmap & IN_STD_BMP) 
        tbActual[i].iBitmap = (tbActual[i].iBitmap & ~IN_STD_BMP) + iStdBMOffset; 
      else
      {
        tbActual[i].iBitmap = tbActual[i].iBitmap + aMyCommandsOffset; 
        if(tbActual[i].iString >= 0 /*&& tbActual[i].iString <= kNumString*/)
          tbActual[i].iString = aStringIndex[tbActual[i].iString]; // aStringIndex; 
      }
    } 
  } 
  
  m_ShellBrowser->SetToolbarItems(tbActual, aNumButtons, FCT_MERGE); 
  
  //CheckToolbar(); 
} 
