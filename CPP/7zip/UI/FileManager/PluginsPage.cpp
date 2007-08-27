// PluginsPage.cpp

#include "StdAfx.h"
#include "PluginsPageRes.h"
#include "PluginsPage.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "Windows/Defs.h"
#include "Windows/DLL.h"
#include "Windows/Control/ListView.h"
#include "Windows/FileFind.h"

#include "RegistryUtils.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "ProgramLocation.h"

#include "PluginInterface.h"

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_PLUGINS_STATIC_PLUGINS, 0x03010101},
  { IDC_PLUGINS_BUTTON_OPTIONS, 0x03010110}
};

static LPCWSTR kPluginsTopic = L"FM/options.htm#plugins";

bool CPluginsPage::OnInit()
{
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _listView.Attach(GetItem(IDC_PLUGINS_LIST));

  UINT32 newFlags = /*LVS_EX_CHECKBOXES | */ LVS_EX_FULLROWSELECT;
  _listView.SetExtendedListViewStyle(newFlags, newFlags);

  UString title = L"Plugins";
  LVCOLUMNW column;
  column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  column.cx = 160;
  column.fmt = LVCFMT_LEFT;
  column.pszText = (LPWSTR)(LPCWSTR)title;
  column.iSubItem = 0;
  _listView.InsertColumn(0, &column);
  
  ReadFileFolderPluginInfoList(_plugins);

  _listView.SetRedraw(false);
  // _listView.DeleteAllItems();
  for(int i = 0; i < _plugins.Size(); i++)
  {
    const CPluginInfo &p = _plugins[i];
    if (!p.OptionsClassIDDefined)
      continue;
    LVITEMW item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_STATE;
    UString pluginName = p.Name;
    item.pszText = (WCHAR *)(const WCHAR *)pluginName;
    item.state = 0;
    item.stateMask = UINT(-1);
    item.iSubItem = 0;
    _listView.InsertItem(&item);
    _listView.SetCheckState(i, true);
  }
  _listView.SetRedraw(true);
  if(_listView.GetItemCount() > 0)
  {
    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    _listView.SetItemState(0, state, state);
  }

  return CPropertyPage::OnInit();
}

LONG CPluginsPage::OnApply()
{
  /*
  int selectedIndex = m_Lang.GetCurSel();
  int aPathIndex = m_Lang.GetItemData(selectedIndex);
  SaveRegLang(m_Paths[aPathIndex]);
  ReloadLang();
  */
  return PSNRET_NOERROR;
}

void CPluginsPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kPluginsTopic);
}

bool CPluginsPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    case IDC_PLUGINS_BUTTON_OPTIONS:
      OnButtonOptions();
      break;
    default:
      return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
  }
  return true;
}

class CPluginOptionsCallback: 
  public IPluginOptionsCallback,
  public CMyUnknownImp
{
  UString _pluginName;
public:
  MY_UNKNOWN_IMP

  STDMETHOD(GetProgramFolderPath)(BSTR *value);  
  STDMETHOD(GetProgramPath)(BSTR *Value);  
  STDMETHOD(GetRegistryCUPath)(BSTR *Value);  
  void Init(const UString &pluginName)
    { _pluginName = pluginName; }
};

STDMETHODIMP CPluginOptionsCallback::GetProgramFolderPath(BSTR *value)
{
  *value = 0;
  UString folder;
  if (!::GetProgramFolderPath(folder))
    return E_FAIL;
  CMyComBSTR valueTemp = folder;
  *value = valueTemp.Detach();
  return S_OK;
}

static UString GetDefaultProgramName()
{
  return L"7zFM.exe";
}

STDMETHODIMP CPluginOptionsCallback::GetProgramPath(BSTR *value)
{
  *value = 0;
  UString folder;
  if (!::GetProgramFolderPath(folder))
    return E_FAIL;
  CMyComBSTR valueTemp = folder + GetDefaultProgramName();
  *value = valueTemp.Detach();
  return S_OK;
}

STDMETHODIMP CPluginOptionsCallback::GetRegistryCUPath(BSTR *value)
{
  CMyComBSTR valueTemp = UString(L"Software\\7-Zip\\FM\\Plugins\\") + _pluginName;
  *value = valueTemp.Detach();
  return S_OK;
}

void CPluginsPage::OnButtonOptions()
{
  int index = _listView.GetSelectionMark();
  if (index < 0)
    return;

  CPluginInfo pluginInfo = _plugins[index];
  if (!pluginInfo.OptionsClassIDDefined)
  {
    MessageBoxW(HWND(*this), L"There are no options", L"7-Zip", 0);
    return;
  }
  NWindows::NDLL::CLibrary library;
  CMyComPtr<IPluginOptions> pluginOptions;
  if (!library.Load(pluginInfo.FilePath))
  {
    MessageBoxW(HWND(*this), L"Can't load plugin", L"7-Zip", 0);
    return;
  }
  typedef UINT32 (WINAPI * CreateObjectPointer)(
      const GUID *clsID, const GUID *interfaceID, void **outObject);
  CreateObjectPointer createObject = (CreateObjectPointer)
        library.GetProcAddress("CreateObject");
  if (createObject == NULL)
  {
    MessageBoxW(HWND(*this), L"Incorrect plugin", L"7-Zip", 0);
    return;
  }
  if (createObject(&pluginInfo.OptionsClassID, &IID_IPluginOptions, (void **)&pluginOptions) != S_OK)
  {
    MessageBoxW(HWND(*this), L"There are no options", L"7-Zip", 0);
    return;
  }
  CPluginOptionsCallback *callbackSpec = new CPluginOptionsCallback;
  CMyComPtr<IPluginOptionsCallback> callback(callbackSpec);
  callbackSpec->Init(pluginInfo.Name);
  pluginOptions->PluginOptions(HWND(*this), callback);
}

bool CPluginsPage::OnNotify(UINT controlID, LPNMHDR lParam) 
{ 
  if (lParam->hwndFrom == HWND(_listView) && lParam->code == LVN_ITEMCHANGED)
  {
    const NMLISTVIEW *aNMListView = (const NMLISTVIEW *)lParam;
    if ((aNMListView->uChanged & LVIF_STATE) != 0)
    {
      UINT oldState = aNMListView->uOldState & LVIS_STATEIMAGEMASK;
      UINT newState = aNMListView->uNewState & LVIS_STATEIMAGEMASK;
      if (oldState != newState)
        Changed();
    }
    return true;
  }
  return CPropertyPage::OnNotify(controlID, lParam); 
}

/*
bool CPluginsPage::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == CBN_SELCHANGE && itemID == IDC_LANG_COMBO_LANG)
  {
    Changed();
    return true;
  }
  return CPropertyPage::OnCommand(code, itemID, lParam);
}

*/