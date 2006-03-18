#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Windows/Menu.h"
#include "Windows/COM.h"

#include "Panel.h"
#include "PluginInterface.h"
#include "MyLoadMenu.h"
#include "App.h"
#include "LangUtils.h"
#include "resource.h"

using namespace NWindows;

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

static const UINT kSevenZipStartMenuID = kPluginMenuStartID ;
static const UINT kSystemStartMenuID = kPluginMenuStartID + 100;

void CPanel::InvokeSystemCommand(const char *command)
{
  if (!IsFSFolder() && !IsFSDrivesFolder())
    return;
  CRecordVector<UINT32> operatedIndices;
  GetOperatedItemIndices(operatedIndices);
  if (operatedIndices.IsEmpty())
    return;
  CMyComPtr<IContextMenu> contextMenu;
  if (CreateShellContextMenu(operatedIndices, contextMenu) != S_OK)
    return;

  CMINVOKECOMMANDINFO ci;
  ZeroMemory(&ci, sizeof(ci));
  ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
  ci.hwnd = GetParent();
  ci.lpVerb = command;
  contextMenu->InvokeCommand(&ci);
}

void CPanel::Properties()
{
  InvokeSystemCommand("properties");
}

// Copy and paste do not work, if you know why write me.

void CPanel::EditCopy()
{
  NCOM::CComInitializer comInitializer;
  InvokeSystemCommand("copy");
}

void CPanel::EditPaste()
{
  NCOM::CComInitializer comInitializer;
  InvokeSystemCommand("paste");
}

HRESULT CPanel::CreateShellContextMenu(
    const CRecordVector<UINT32> &operatedIndices,
    CMyComPtr<IContextMenu> &systemContextMenu)
{
  systemContextMenu.Release();
  UString folderPath = GetFsPath();

  CMyComPtr<IShellFolder> desktopFolder;
  RINOK(::SHGetDesktopFolder(&desktopFolder));
  if (!desktopFolder) 
  {
    // ShowMessage("Failed to get Desktop folder.");
    return E_FAIL;
  }
  
  // Separate the file from the folder.

  
  // Get a pidl for the folder the file
  // is located in.
  LPITEMIDLIST parentPidl;
  DWORD eaten;
  RINOK(desktopFolder->ParseDisplayName(
      GetParent(), 0, (wchar_t *)(const wchar_t *)folderPath, 
      &eaten, &parentPidl, 0));
  
  // Get an IShellFolder for the folder
  // the file is located in.
  CMyComPtr<IShellFolder> parentFolder;
  RINOK(desktopFolder->BindToObject(parentPidl,
      0, IID_IShellFolder, (void**)&parentFolder));
  if (!parentFolder) 
  {
    // ShowMessage("Invalid file name.");
    return E_FAIL;
  }
  
  // Get a pidl for the file itself.
  CRecordVector<LPITEMIDLIST> pidls;
  pidls.Reserve(operatedIndices.Size());
  for (int i = 0; i < operatedIndices.Size(); i++)
  {
    LPITEMIDLIST pidl;
    UString fileName = GetItemRelPath(operatedIndices[i]);
    if (IsFSDrivesFolder())
      fileName += L'\\';
    RINOK(parentFolder->ParseDisplayName(GetParent(), 0, 
      (wchar_t *)(const wchar_t *)fileName, &eaten, &pidl, 0));
    pidls.Add(pidl);
  }

  ITEMIDLIST temp;
  if (pidls.Size() == 0)
  {
    temp.mkid.cb = 0;
    /*
    LPITEMIDLIST pidl;
    HRESULT result = parentFolder->ParseDisplayName(GetParent(), 0, 
      L".\\", &eaten, &pidl, 0);
    if (result != NOERROR)
      return;
    */
    pidls.Add(&temp);
  }

  // Get the IContextMenu for the file.
  CMyComPtr<IContextMenu> cm;
  RINOK( parentFolder->GetUIObjectOf(GetParent(), pidls.Size(), 
      (LPCITEMIDLIST *)&pidls.Front(), IID_IContextMenu, 0, (void**)&cm));
  if (!cm) 
  {
    // ShowMessage("Unable to get context menu interface.");
    return E_FAIL;
  }
  systemContextMenu = cm;
  return S_OK;
}

void CPanel::CreateSystemMenu(HMENU menuSpec, 
    const CRecordVector<UINT32> &operatedIndices,
    CMyComPtr<IContextMenu> &systemContextMenu)
{
  systemContextMenu.Release();

  CreateShellContextMenu(operatedIndices, systemContextMenu);

  if (systemContextMenu == 0)
    return;
  
  // Set up a CMINVOKECOMMANDINFO structure.
  CMINVOKECOMMANDINFO ci;
  ZeroMemory(&ci, sizeof(ci));
  ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
  ci.hwnd = GetParent();
  
  /*
  if (Sender == GoBtn) 
  {
    // Verbs that can be used are cut, paste,
    // properties, delete, and so on.
    String action;
    if (CutRb->Checked)
      action = "cut";
    else if (CopyRb->Checked)
      action = "copy";
    else if (DeleteRb->Checked)
      action = "delete";
    else if (PropertiesRb->Checked)
      action = "properties";
    
    ci.lpVerb = action.c_str();
    result = cm->InvokeCommand(&ci);
    if (result)
      ShowMessage(
      "Error copying file to clipboard.");
    
  } 
  else 
  */
  {
    // HMENU hMenu = CreatePopupMenu();
    CMenu popupMenu;
    // CMenuDestroyer menuDestroyer(popupMenu);
    if(!popupMenu.CreatePopup())
      throw 210503;

    HMENU hMenu = popupMenu;

    DWORD Flags = CMF_EXPLORE;
    // Optionally the shell will show the extended
    // context menu on some operating systems when
    // the shift key is held down at the time the
    // context menu is invoked. The following is
    // commented out but you can uncommnent this
    // line to show the extended context menu.
    // Flags |= 0x00000080;
    systemContextMenu->QueryContextMenu(hMenu, 0, kSystemStartMenuID, 0x7FFF, Flags);
    

    {
      CMenu menu;
      menu.Attach(menuSpec);
      CMenuItem menuItem;
      menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
      menuItem.fType = MFT_STRING;
      menuItem.hSubMenu = popupMenu.Detach();
      // menuDestroyer.Disable();
      menuItem.StringValue = LangString(IDS_SYSTEM, 0x030202A0);
      menu.InsertItem(0, true, menuItem);
    }
    /*
    if (Cmd < 100 && Cmd != 0) 
    {
      ci.lpVerb = MAKEINTRESOURCE(Cmd - 1);
      ci.lpParameters = "";
      ci.lpDirectory = "";
      ci.nShow = SW_SHOWNORMAL;
      cm->InvokeCommand(&ci);
    }
    // If Cmd is > 100 then it's one of our
    // inserted menu items.
    else
      // Find the menu item.
      for (int i = 0; i < popupMenu1->Items->Count; i++) 
      {
        TMenuItem* menu = popupMenu1->Items->Items[i];
        // Call its OnClick handler.
        if (menu->Command == Cmd - 100)
          menu->OnClick(this);
      }
      // Release the memory allocated for the menu.
      DestroyMenu(hMenu);
    */
  }
}

void CPanel::CreateFileMenu(HMENU menuSpec)
{
  CreateFileMenu(menuSpec, _sevenZipContextMenu, _systemContextMenu, true);
}

void CPanel::CreateSevenZipMenu(HMENU menuSpec, 
    const CRecordVector<UINT32> &operatedIndices,
    CMyComPtr<IContextMenu> &sevenZipContextMenu)
{
  sevenZipContextMenu.Release();

  CMenu menu;
  menu.Attach(menuSpec);
  // CMenuDestroyer menuDestroyer(menu);
  // menu.CreatePopup();

  bool sevenZipMenuCreated = false;

  CMyComPtr<IContextMenu> contextMenu;
  if (contextMenu.CoCreateInstance(CLSID_CZipContextMenu, IID_IContextMenu) == S_OK)
  {
    CMyComPtr<IInitContextMenu> initContextMenu;
    if (contextMenu.QueryInterface(IID_IInitContextMenu, &initContextMenu) != S_OK)
      return;
    UString currentFolderUnicode = _currentFolderPrefix;
    UStringVector names;
    int i;
    for(i = 0; i < operatedIndices.Size(); i++)
      names.Add(currentFolderUnicode + GetItemRelPath(operatedIndices[i]));
    CRecordVector<const wchar_t *> namePointers;
    for(i = 0; i < operatedIndices.Size(); i++)
      namePointers.Add(names[i]);
    
    // NFile::NDirectory::MySetCurrentDirectory(currentFolderUnicode);
    if (initContextMenu->InitContextMenu(currentFolderUnicode, &namePointers.Front(),
        operatedIndices.Size()) == S_OK)
    {
      HRESULT res = contextMenu->QueryContextMenu(menu, 0, kSevenZipStartMenuID, 
          kSystemStartMenuID - 1, 0);
      sevenZipMenuCreated = (HRESULT_SEVERITY(res) == SEVERITY_SUCCESS);
      if (sevenZipMenuCreated)
        sevenZipContextMenu = contextMenu;
      // int code = HRESULT_CODE(res);
      // int nextItemID = code;
    }
  }
}

void CPanel::CreateFileMenu(HMENU menuSpec, 
    CMyComPtr<IContextMenu> &sevenZipContextMenu,
    CMyComPtr<IContextMenu> &systemContextMenu,
    bool programMenu)
{
  sevenZipContextMenu.Release();
  systemContextMenu.Release();

  CRecordVector<UINT32> operatedIndices;
  GetOperatedItemIndices(operatedIndices);

  CMenu menu;
  menu.Attach(menuSpec);
  
  CreateSevenZipMenu(menu, operatedIndices, sevenZipContextMenu);
  if (g_App.ShowSystemMenu)
    CreateSystemMenu(menu, operatedIndices, systemContextMenu);

  if (menu.GetItemCount() > 0)
    menu.AppendItem(MF_SEPARATOR, 0, (LPCTSTR)0);

  LoadFileMenu(menu, menu.GetItemCount(), !operatedIndices.IsEmpty(), programMenu);
}

bool CPanel::InvokePluginCommand(int id)
{
  return InvokePluginCommand(id, _sevenZipContextMenu, _systemContextMenu);
}

bool CPanel::InvokePluginCommand(int id, 
    IContextMenu *sevenZipContextMenu, IContextMenu *systemContextMenu)
{
  UINT32 offset;
  bool isSystemMenu = (id >= kSystemStartMenuID);
  if (isSystemMenu)
    offset = id  - kSystemStartMenuID;
  else
    offset = id  - kSevenZipStartMenuID;

  CMINVOKECOMMANDINFOEX commandInfo;
  commandInfo.cbSize = sizeof(commandInfo);
  commandInfo.fMask = CMIC_MASK_UNICODE;
  commandInfo.hwnd = GetParent();
  commandInfo.lpVerb = LPCSTR(offset);
  commandInfo.lpParameters = NULL;
  CSysString currentFolderSys = GetSystemString(_currentFolderPrefix);
  commandInfo.lpDirectory = (LPCSTR)(LPCTSTR)(currentFolderSys);
  commandInfo.nShow = SW_SHOW;
  commandInfo.lpTitle = "";
  commandInfo.lpVerbW = LPCWSTR(offset);
  commandInfo.lpParameters = NULL;
  UString currentFolderUnicode = _currentFolderPrefix;
  commandInfo.lpDirectoryW = currentFolderUnicode;
  commandInfo.lpTitleW = L"";
  // commandInfo.ptInvoke.x = xPos;
  // commandInfo.ptInvoke.y = yPos;
  commandInfo.ptInvoke.x = 0;
  commandInfo.ptInvoke.y = 0;
  HRESULT result;
  if (isSystemMenu)
    result = systemContextMenu->InvokeCommand(LPCMINVOKECOMMANDINFO(&commandInfo));
  else
    result = sevenZipContextMenu->InvokeCommand(LPCMINVOKECOMMANDINFO(&commandInfo));
  if (result == NOERROR)
  {
    KillSelection();
    return true;
  }
  return false;
}

bool CPanel::OnContextMenu(HANDLE windowHandle, int xPos, int yPos)
{
  if (windowHandle != _listView)
    return false;
  /*
  POINT point;
  point.x = xPos;
  point.y = yPos;
  if (!_listView.ScreenToClient(&point))
    return false;

  LVHITTESTINFO info;
  info.pt = point;
  int index = _listView.HitTest(&info);
  */

  CRecordVector<UINT32> operatedIndices;
  GetOperatedItemIndices(operatedIndices);

  if (xPos < 0 || yPos < 0)
  {
    if (operatedIndices.Size() == 0)
    {
      xPos = 0;
      yPos = 0;
    }
    else
    {
      int itemIndex = _listView.GetNextItem(-1, LVNI_FOCUSED);
      if (itemIndex == -1)
        return false;
      RECT rect;
      if (!_listView.GetItemRect(itemIndex, &rect, LVIR_ICON))
        return false;
      xPos = (rect.left + rect.right) / 2;
      yPos = (rect.top + rect.bottom) / 2;
    }
    POINT point = {xPos, yPos};
    _listView.ClientToScreen(&point);
    xPos = point.x;
    yPos = point.y;
  }

  CMenu menu;
  CMenuDestroyer menuDestroyer(menu);
  menu.CreatePopup();

  CMyComPtr<IContextMenu> sevenZipContextMenu;
  CMyComPtr<IContextMenu> systemContextMenu;
  CreateFileMenu(menu, sevenZipContextMenu, systemContextMenu, false);

  int result = menu.Track(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, 
    xPos, yPos, _listView);

  if (result == 0)
    return true;

  if (result >= kPluginMenuStartID)
  {
    InvokePluginCommand(result, sevenZipContextMenu, systemContextMenu);
    return true;
  }
  if (ExecuteFileCommand(result))
    return true;
  return true;
}
