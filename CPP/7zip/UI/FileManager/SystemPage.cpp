// SystemPage.cpp

#include "StdAfx.h"
#include "SystemPageRes.h"
#include "SystemPage.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "Windows/Defs.h"
#include "Windows/Control/ListView.h"
#include "Windows/FileFind.h"

#include "IFolder.h"
#include "HelpUtils.h"
#include "LangUtils.h"
#include "PluginLoader.h"
#include "ProgramLocation.h"
#include "StringUtils.h"

#include "PropertyNameRes.h"
#include "../Agent/Agent.h"

using namespace NRegistryAssociations;

const int kRefreshpluginsListMessage  = WM_USER + 1;
const int kUpdateDatabase = kRefreshpluginsListMessage  + 1;

static CIDLangPair kIDLangPairs[] = 
{
  { IDC_SYSTEM_STATIC_ASSOCIATE,  0x03010302},
  { IDC_SYSTEM_SELECT_ALL,        0x03000330}
};

static LPCWSTR kSystemTopic = L"FM/options.htm#system";


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

  UINT32 newFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  _listViewExt.SetExtendedListViewStyle(newFlags, newFlags);
  _listViewPlugins.SetExtendedListViewStyle(newFlags, newFlags);

  UString s = LangString(IDS_PROPERTY_EXTENSION, 0x02000205);
  LVCOLUMNW column;
  column.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  column.cx = 70;
  column.fmt = LVCFMT_LEFT;
  column.pszText = (LPWSTR)(LPCWSTR)s;
  column.iSubItem = 0;
  _listViewExt.InsertColumn(0, &column);

  s = LangString(IDS_PLUGIN, 0x03010310);
  column.cx = 70;
  column.pszText = (LPWSTR)(LPCWSTR)s;
  column.iSubItem = 1;
  _listViewExt.InsertColumn(1, &column);

  s = LangString(IDS_PLUGIN, 0x03010310);
  column.cx = 70;
  column.pszText = (LPWSTR)(LPCWSTR)s;
  column.iSubItem = 0;
  _listViewPlugins.InsertColumn(0, &column);

  _extDatabase.Read();

  for (int i = 0; i < _extDatabase.ExtBigItems.Size(); i++)
  {
    CExtInfoBig &extInfo = _extDatabase.ExtBigItems[i];

    LVITEMW item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.lParam = i;
    item.pszText = (LPWSTR)(LPCWSTR)extInfo.Ext;
    item.iSubItem = 0;
    int itemIndex = _listViewExt.InsertItem(&item);

    UString iconPath;
    int iconIndex;
    extInfo.Associated = NRegistryAssociations::CheckShellExtensionInfo(GetSystemString(extInfo.Ext), iconPath, iconIndex);
    if (extInfo.Associated && !NWindows::NFile::NFind::DoesFileExist(iconPath))
      extInfo.Associated = false;
    _listViewExt.SetCheckState(itemIndex, extInfo.Associated);

    SetMainPluginText(itemIndex, i);
  }
  // _listViewExt.SortItems();
  
  if(_listViewExt.GetItemCount() > 0)
  {
    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    _listViewExt.SetItemState(0, state, state);
  }
  RefreshPluginsList(-1);
  _initMode = false;

  return CPropertyPage::OnInit();
}

void CSystemPage::SetMainPluginText(int itemIndex, int indexInDatabase)
{
  LVITEMW item;
  item.iItem = itemIndex;
  item.mask = LVIF_TEXT;
  UString mainPlugin = _extDatabase.GetMainPluginNameForExtItem(indexInDatabase);
  item.pszText = (WCHAR *)(const WCHAR *)mainPlugin;
  item.iSubItem = 1;
  _listViewExt.SetItem(&item);
}

static UString GetProgramCommand()
{
  UString path = L"\"";
  UString folder;
  if (GetProgramFolderPath(folder))
    path += folder;
  path += L"7zFM.exe\" \"%1\"";
  return path;
}

static UString GetIconPath(const UString &filePath,
    const CLSID &clsID, const UString &extension, Int32 &iconIndex)
{
  CPluginLibrary library;
  CMyComPtr<IFolderManager> folderManager;
  CMyComPtr<IFolderFolder> folder;
  if (filePath.IsEmpty())
    folderManager = new CArchiveFolderManager;
  else if (library.LoadAndCreateManager(filePath, clsID, &folderManager) != S_OK)
    return UString();
  CMyComBSTR extBSTR;
  if (folderManager->GetExtensions(&extBSTR) != S_OK)
    return UString();
  const UString ext2 = (const wchar_t *)extBSTR;
  UStringVector exts;
  SplitString(ext2, exts);
  for (int i = 0; i < exts.Size(); i++)
  {
    const UString &plugExt = exts[i];
    if (extension.CompareNoCase((const wchar_t *)plugExt) == 0)
    {
      CMyComBSTR iconPathTemp;
      if (folderManager->GetIconPath(plugExt, &iconPathTemp, &iconIndex) != S_OK)
        break;
      if (iconPathTemp != 0)
        return (const wchar_t *)iconPathTemp;
    }
  }
  return UString();
}

LONG CSystemPage::OnApply()
{
  UpdateDatabase();
  _extDatabase.Save();
  UString command = GetProgramCommand();
  
  for (int i = 0; i < _extDatabase.ExtBigItems.Size(); i++)
  {
    const CExtInfoBig &extInfo = _extDatabase.ExtBigItems[i];
    if (extInfo.Associated)
    {
      UString title = extInfo.Ext + UString(L" Archive");
      UString command = GetProgramCommand();
      UString iconPath;
      Int32 iconIndex = -1;
      if (!extInfo.PluginsPairs.IsEmpty())
      {
        const CPluginInfo &plugin = _extDatabase.Plugins[extInfo.PluginsPairs[0].Index];
        iconPath = GetIconPath(plugin.FilePath, plugin.ClassID, extInfo.Ext, iconIndex);
      }
      NRegistryAssociations::AddShellExtensionInfo(GetSystemString(extInfo.Ext), 
            title, command, iconPath, iconIndex, NULL, 0);
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
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  return PSNRET_NOERROR;
}

void CSystemPage::OnNotifyHelp()
{
  ShowHelpWindow(NULL, kSystemTopic);
}

void CSystemPage::SelectAll()
{ 
  int count = _listViewExt.GetItemCount();
  for (int i = 0; i < count; i++)
    _listViewExt.SetCheckState(i, true);
  UpdateDatabase();
}

bool CSystemPage::OnButtonClicked(int buttonID, HWND buttonHWND)
{ 
  switch(buttonID)
  {
    case IDC_SYSTEM_SELECT_ALL:
    {
      SelectAll();
      Changed();
      return true;
    }
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
  bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  // bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  switch(keyDownInfo->wVKey)
  {
    case VK_UP:
    {
      if (alt)
        MovePlugin(true);
      return true;
    }
    case VK_DOWN:
    {
      if (alt)
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
  CExtInfoBig &extInfo = _extDatabase.ExtBigItems[selectedExtIndex];
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
    UINT oldState = info->uOldState & LVIS_STATEIMAGEMASK;
    UINT newState = info->uNewState & LVIS_STATEIMAGEMASK;
    if (oldState != newState)
      Changed();
  }
  // PostMessage(kRefreshpluginsListMessage, 0);
  // RefreshPluginsList();
  return true;
}

bool CSystemPage::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
    CExtInfoBig &extInfo = _extDatabase.ExtBigItems[(int)param];
    extInfo.Associated = _listViewExt.GetCheckState(i);
  }

  int selectedExtIndex = GetSelectedExtIndex();
  if (selectedExtIndex < 0)
    return;

  CExtInfoBig &extInfo = _extDatabase.ExtBigItems[selectedExtIndex];
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
  return (int)param;
}


void CSystemPage::RefreshPluginsList(int selectIndex)
{
  _listViewPlugins.DeleteAllItems();
  int selectedExtIndex = GetSelectedExtIndex();
  if (selectedExtIndex < 0)
    return;
  const CExtInfoBig &extInfo = _extDatabase.ExtBigItems[selectedExtIndex];

  _initMode = true;
  for (int i = 0; i < extInfo.PluginsPairs.Size(); i++)
  {
    CPluginEnabledPair pluginPair = extInfo.PluginsPairs[i];
    UString pluginName = _extDatabase.Plugins[pluginPair.Index].Name;
    LVITEMW item;
    item.iItem = i;
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.lParam = i;
    item.pszText = (LPWSTR)(LPCWSTR)pluginName;
    item.iSubItem = 0;
    int itemIndex = _listViewPlugins.InsertItem(&item);
    _listViewPlugins.SetCheckState(itemIndex, pluginPair.Enabled);
  }
  if(_listViewPlugins.GetItemCount() > 0)
  {
    if (selectIndex < 0)
      selectIndex = 0;
    UINT state = LVIS_SELECTED | LVIS_FOCUSED;
    _listViewPlugins.SetItemState(selectIndex, state, state);
  }
  _initMode = false;
}



/*
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
