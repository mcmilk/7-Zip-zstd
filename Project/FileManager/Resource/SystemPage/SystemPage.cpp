// SystemDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "SystemPage.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"

#include "../../HelpUtils.h"
#include "../../LangUtils.h"

#include "../../ProgramLocation.h"
#include "../../FolderInterface.h"
#include "../../StringUtils.h"

#include "../PropertyName/resource.h"

using namespace NRegistryAssociations;

const int kRefreshpluginsListMessage  = WM_USER + 1;
const int kUpdateDatabase = kRefreshpluginsListMessage  + 1;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SYSTEM_STATIC_ASSOCIATE,          0x03010302}
};

static LPCTSTR kSystemTopic = _T("FM/options.htm#system");


bool CSystemPage::OnInit()
{
  _initMode = true;
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));

  _listViewExt.Attach(GetItem(IDC_SYSTEM_LIST_ASSOCIATE));
  _listViewPlugins.Attach(GetItem(IDC_SYSTEM_LIST_PLUGINS));

  /*
  CheckButton(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU, 
      NRegistryAssociations::CheckContextMenuHandler());
  */

  UINT32 aNewFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listViewExt.SetExtendedListViewStyle(aNewFlags, aNewFlags);
  _listViewPlugins.SetExtendedListViewStyle(aNewFlags, aNewFlags);

  CSysString aString = LangLoadString(IDS_PROPERTY_EXTENSION, 0x02000205);
  LVCOLUMN aColumn;
  aColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  aColumn.cx = 70;
  aColumn.fmt = LVCFMT_LEFT;
  aColumn.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumn.iSubItem = 0;
  _listViewExt.InsertColumn(0, &aColumn);

  aString = LangLoadString(IDS_PLUGIN, 0x03010310);
  aColumn.cx = 70;
  aColumn.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumn.iSubItem = 1;
  _listViewExt.InsertColumn(1, &aColumn);

  aString = LangLoadString(IDS_PLUGIN, 0x03010310);
  aColumn.cx = 70;
  aColumn.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumn.iSubItem = 0;
  _listViewPlugins.InsertColumn(0, &aColumn);

  _extDatabase.Read();

  for (int i = 0; i < _extDatabase._extBigItems.Size(); i++)
  {
    CExtInfoBig &extInfo = _extDatabase._extBigItems[i];

    LVITEM item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.lParam = i;
    CSysString ext = GetSystemString(extInfo.Ext);
    item.pszText = (LPTSTR)(LPCTSTR)ext;
    item.iSubItem = 0;
    int itemIndex = _listViewExt.InsertItem(&item);

    extInfo.Associated = NRegistryAssociations::CheckShellExtensionInfo(GetSystemString(extInfo.Ext));
    _listViewExt.SetCheckState(itemIndex, extInfo.Associated);

    SetMainPluginText(itemIndex, i);
  }
  // _listViewExt.SortItems();
  
  if(_listViewExt.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    _listViewExt.SetItemState(0, aState, aState);
  }
  RefreshPluginsList(-1);
  _initMode = false;

  return CPropertyPage::OnInit();
}

void CSystemPage::SetMainPluginText(int itemIndex, int indexInDatabase)
{
  LVITEM item;
  item.iItem = itemIndex;
  item.mask = LVIF_TEXT;
  CSysString mainPlugin = GetSystemString(
      _extDatabase.GetMainPluginNameForExtItem(indexInDatabase));
  item.pszText = (TCHAR *)(const TCHAR *)mainPlugin;
  item.iSubItem = 1;
  _listViewExt.SetItem(&item);
}

static bool IsItWindowsNT()
{
  OSVERSIONINFO aVersionInfo;
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!::GetVersionEx(&aVersionInfo)) 
    return false;
  return (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

CSysString GetProgramCommand()
{
  CSysString path = TEXT("\"");
  CSysString folder;
  if (GetProgramFolderPath(folder))
    path += folder;
  if (IsItWindowsNT())
    path += TEXT("7zFMn.exe");
  else
    path += TEXT("7zFM.exe");
  path += TEXT("\" \"%1\"");
  return path;
}

CSysString GetIconPath(const CLSID &ckassID, const UString &extension)
{
  CComPtr<IFolderManager> folderManager;
  if (folderManager.CoCreateInstance(ckassID) != S_OK)
    return CSysString();
  CComBSTR typesString;
  if (folderManager->GetTypes(&typesString) != S_OK)
    return CSysString();
  UStringVector types;
  SplitString((const wchar_t *)typesString, types);
  for (int typeIndex = 0; typeIndex < types.Size(); typeIndex++)
  {
    const UString &type = types[typeIndex];
    CComBSTR extTemp;
    if (folderManager->GetExtension(type, &extTemp) != S_OK)
      continue;
    if (extension.CompareNoCase((const wchar_t *)extTemp) == 0)
    {
      CComPtr<IFolderManagerGetIconPath> getIconPath;
      if (folderManager.QueryInterface(&getIconPath) != S_OK)
        return CSysString();
      CComBSTR iconPathTemp;
      if (getIconPath->GetIconPath(type, &iconPathTemp) != S_OK)
        return CSysString();
      return GetSystemString((const wchar_t *)iconPathTemp);
    }
  }
  return CSysString();
}

LONG CSystemPage::OnApply()
{
  _extDatabase.Save();
  CSysString command = GetProgramCommand();
  
  for (int i = 0; i < _extDatabase._extBigItems.Size(); i++)
  {
    const CExtInfoBig &extInfo = _extDatabase._extBigItems[i];
    if (extInfo.Associated)
    {
      CSysString title = GetSystemString(extInfo.Ext) + CSysString(_T(" Archive"));
      CSysString command = GetProgramCommand();
      CSysString iconPath;
      if (!extInfo.PluginsPairs.IsEmpty())
          iconPath = GetIconPath(_extDatabase.
          _plugins[extInfo.PluginsPairs[0].Index].ClassID, extInfo.Ext);
        NRegistryAssociations::AddShellExtensionInfo(
            GetSystemString(extInfo.Ext), title, command, iconPath, NULL, 0);
    }
    else
      NRegistryAssociations::DeleteShellExtensionInfo(GetSystemString(extInfo.Ext));
  }
  /*
  if (IsButtonCheckedBool(IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU))
    NRegistryAssociations::AddContextMenuHandler();
  else
    NRegistryAssociations::DeleteContextMenuHandler();
  */

  return PSNRET_NOERROR;
}

void CSystemPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic);
}

bool CSystemPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{ 
  switch(buttonID)
  {
    case IDC_SYSTEM_INTEGRATE_TO_CONTEXT_MENU:
      Changed();
      return true;
  }
  return CPropertyPage::OnButtonClicked(buttonID, buttonHWND);
}

bool CSystemPage::OnNotify(UINT controlID, LPNMHDR lParam) 
{ 
  if (lParam->hwndFrom == HWND(_listViewExt))
  {
    switch(lParam->code)
    {
      case (LVN_ITEMCHANGED):
        return OnItemChanged((const NMLISTVIEW *)lParam);
      case NM_RCLICK:
      case NM_DBLCLK:
      case LVN_KEYDOWN:
      case NM_CLICK:
      case LVN_BEGINRDRAG:
        PostMessage(kRefreshpluginsListMessage, 0);
        PostMessage(kUpdateDatabase, 0);
        break;
    }
  } 
  else if (lParam->hwndFrom == HWND(_listViewPlugins))
  {
    switch(lParam->code)
    {
      case NM_RCLICK:
      case NM_DBLCLK:
      // case LVN_KEYDOWN:
      case NM_CLICK:
      case LVN_BEGINRDRAG:
        PostMessage(kUpdateDatabase, 0);
        break;

      case (LVN_ITEMCHANGED):
      {
        OnItemChanged((const NMLISTVIEW *)lParam);
        PostMessage(kUpdateDatabase, 0);
        break;
      }
      case LVN_KEYDOWN:
      {
        OnPluginsKeyDown((LPNMLVKEYDOWN)lParam);
        PostMessage(kUpdateDatabase, 0);
        break;
      }
    }
  }
  return CPropertyPage::OnNotify(controlID, lParam); 
}

bool CSystemPage::OnPluginsKeyDown(LPNMLVKEYDOWN keyDownInfo)
{
  bool anAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  // bool aCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  switch(keyDownInfo->wVKey)
  {
    case VK_UP:
    {
      if (anAlt)
        MovePlugin(true);
      return true;
    }
    case VK_DOWN:
    {
      if (anAlt)
        MovePlugin(false);
      return true;
    }
  }
  return false;
}

void CSystemPage::MovePlugin(bool upDirection)
{
  int selectedPlugin = _listViewPlugins.GetSelectionMark();
  if (selectedPlugin < 0)
    return;
  int newIndex = selectedPlugin + (upDirection ? -1: 1);
  if (newIndex < 0 || newIndex >= _listViewPlugins.GetItemCount())
    return;
  int selectedExtIndex = GetSelectedExtIndex();
  if (selectedExtIndex < 0)
    return;
  CExtInfoBig &extInfo = _extDatabase._extBigItems[selectedExtIndex];
  CPluginEnabledPair pluginPairTemp = extInfo.PluginsPairs[newIndex];
  extInfo.PluginsPairs[newIndex] = extInfo.PluginsPairs[selectedPlugin];
  extInfo.PluginsPairs[selectedPlugin] = pluginPairTemp;

  SetMainPluginText(_listViewExt.GetSelectionMark(), selectedExtIndex);
  RefreshPluginsList(newIndex);

  Changed();
}

bool CSystemPage::OnItemChanged(const NMLISTVIEW *info)
{
  if (_initMode)
    return true;
  if ((info->uChanged & LVIF_STATE) != 0)
  {
    UINT anOldState = info->uOldState & LVIS_STATEIMAGEMASK;
    UINT aNewState = info->uNewState & LVIS_STATEIMAGEMASK;
    if (anOldState != aNewState)
      Changed();
  }
  // PostMessage(kRefreshpluginsListMessage, 0);
  // RefreshPluginsList();
  return true;
}

bool CSystemPage::OnMessage(UINT message, UINT wParam, LPARAM lParam)
{
  switch(message)
  {
    case kRefreshpluginsListMessage:
      RefreshPluginsList(-1);
      return true;
    case kUpdateDatabase:
      UpdateDatabase();
      return true;
  }
  return CPropertyPage::OnMessage(message, wParam, lParam);
}

void CSystemPage::UpdateDatabase()
{
  int i;
  for (i = 0; i < _listViewExt.GetItemCount(); i++)
  {
    LPARAM param;
    if (!_listViewExt.GetItemParam(i, param))
      return;
    CExtInfoBig &extInfo = _extDatabase._extBigItems[param];
    extInfo.Associated = _listViewExt.GetCheckState(i);
  }

  int selectedExtIndex = GetSelectedExtIndex();
  if (selectedExtIndex < 0)
    return;

  CExtInfoBig &extInfo = _extDatabase._extBigItems[selectedExtIndex];
  for (i = 0; i < _listViewPlugins.GetItemCount(); i++)
  {
    extInfo.PluginsPairs[i].Enabled = _listViewPlugins.GetCheckState(i);
  }
}



int CSystemPage::GetSelectedExtIndex()
{
  int selectedIndex = _listViewExt.GetSelectionMark();
  if (selectedIndex < 0)
    return -1;
  LPARAM param;
  if (!_listViewExt.GetItemParam(selectedIndex, param))
    return -1;
  return param;
}


void CSystemPage::RefreshPluginsList(int selectIndex)
{
  _listViewPlugins.DeleteAllItems();
  int selectedExtIndex = GetSelectedExtIndex();
  if (selectedExtIndex < 0)
    return;
  const CExtInfoBig &extInfo = _extDatabase._extBigItems[selectedExtIndex];

  _initMode = true;
  for (int i = 0; i < extInfo.PluginsPairs.Size(); i++)
  {
    CPluginEnabledPair pluginPair = extInfo.PluginsPairs[i];
    CSysString pluginName = GetSystemString(_extDatabase._plugins[pluginPair.Index].Name);
    LVITEM item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.lParam = i;
    item.pszText = (LPTSTR)(LPCTSTR)pluginName;
    item.iSubItem = 0;
    int itemIndex = _listViewPlugins.InsertItem(&item);
    _listViewPlugins.SetCheckState(itemIndex, pluginPair.Enabled);
  }
  if(_listViewPlugins.GetItemCount() > 0)
  {
    if (selectIndex < 0)
      selectIndex = 0;
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    _listViewPlugins.SetItemState(selectIndex, aState, aState);
  }
  _initMode = false;
}



/*
static LPCTSTR kZIPExtension = _T("zip");
static LPCTSTR kRARExtension = _T("rar");

static BYTE kZipShellNewData[] =
  { 0x50-1, 0x4B, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0 };

static BYTE kRarShellNewData[] =
  { 0x52-1, 0x61, 0x72, 0x21, 0x1A, 7, 0, 0xCF, 0x90, 0x73, 0, 0, 0x0D, 0, 0, 0, 0, 0, 0, 0};

class CSignatureMaker
{
public:
  CSignatureMaker()
  {
    kZipShellNewData[0]++;
    kRarShellNewData[0]++;
  };
};

static CSignatureMaker g_SignatureMaker;
*/
