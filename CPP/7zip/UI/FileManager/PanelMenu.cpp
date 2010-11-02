#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/Clipboard.h"
#include "Windows/Menu.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../../PropID.h"
#include "../Common/PropIDUtils.h"
#include "../Explorer/ContextMenu.h"

#include "App.h"
#include "LangUtils.h"
#include "MyLoadMenu.h"
#include "PropertyName.h"

#include "resource.h"
#include "PropertyNameRes.h"

using namespace NWindows;

LONG g_DllRefCount = 0;

static const UINT kSevenZipStartMenuID = kPluginMenuStartID ;
static const UINT kSystemStartMenuID = kPluginMenuStartID + 100;

void CPanel::InvokeSystemCommand(const char *command)
{
  NCOM::CComInitializer comInitializer;
  if (!IsFsOrPureDrivesFolder())
    return;
  CRecordVector<UInt32> operatedIndices;
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

static const wchar_t *kSeparator = L"----------------------------\n";
static const wchar_t *kSeparatorSmall = L"----\n";
static const wchar_t *kPropValueSeparator = L": ";

extern UString ConvertSizeToString(UInt64 value);

static void AddPropertyString(PROPID propID, const wchar_t *nameBSTR,
    const NCOM::CPropVariant &prop, UString &s)
{
  if (prop.vt != VT_EMPTY)
  {
    UString val;

    if ((prop.vt == VT_UI8 || prop.vt == VT_UI4) && (
        propID == kpidSize ||
        propID == kpidPackSize ||
        propID == kpidNumSubDirs ||
        propID == kpidNumSubFiles ||
        propID == kpidNumBlocks ||
        propID == kpidClusterSize ||
        propID == kpidTotalSize ||
        propID == kpidFreeSpace ||
        propID == kpidPhySize ||
        propID == kpidHeadersSize ||
        propID == kpidFreeSpace
        ))
      val = ConvertSizeToString(ConvertPropVariantToUInt64(prop));
    else
      val = ConvertPropertyToString(prop, propID);

    if (!val.IsEmpty())
    {
      s += GetNameOfProperty(propID, nameBSTR);
      s += kPropValueSeparator;
      /*
      if (propID == kpidComment)
        s += L'\n';
      */
      s += val;
      s += L'\n';
    }
  }
}

void CPanel::Properties()
{
  CMyComPtr<IGetFolderArcProps> getFolderArcProps;
  _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
  if (!getFolderArcProps)
  {
    InvokeSystemCommand("properties");
    return;
  }
  
  {
    UString message;

    CRecordVector<UInt32> operatedIndices;
    GetOperatedItemIndices(operatedIndices);
    if (operatedIndices.Size() == 1)
    {
      UInt32 index = operatedIndices[0];
      // message += L"Item:\n";
      UInt32 numProps;
      if (_folder->GetNumberOfProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE varType;
          
          if (_folder->GetPropertyInfo(i, &name, &propID, &varType) != S_OK)
            continue;
          
          NCOM::CPropVariant prop;
          if (_folder->GetProperty(index, propID, &prop) != S_OK)
            continue;
          AddPropertyString(propID, name, prop, message);
        }
      }
      message += kSeparator;
    }
        
    /*
    message += LangString(IDS_PROP_FILE_TYPE, 0x02000214);
    message += kPropValueSeparator;
    message += GetFolderTypeID();
    message += L"\n";
    */

    {
      NCOM::CPropVariant prop;
      if (_folder->GetFolderProperty(kpidPath, &prop) == S_OK)
      {
        AddPropertyString(kpidName, L"Path", prop, message);
      }
    }

    CMyComPtr<IFolderProperties> folderProperties;
    _folder.QueryInterface(IID_IFolderProperties, &folderProperties);
    if (folderProperties)
    {
      UInt32 numProps;
      if (folderProperties->GetNumberOfFolderProperties(&numProps) == S_OK)
      {
        for (UInt32 i = 0; i < numProps; i++)
        {
          CMyComBSTR name;
          PROPID propID;
          VARTYPE vt;
          if (folderProperties->GetFolderPropertyInfo(i, &name, &propID, &vt) != S_OK)
            continue;
          NCOM::CPropVariant prop;
          if (_folder->GetFolderProperty(propID, &prop) != S_OK)
            continue;
          AddPropertyString(propID, name, prop, message);
        }
      }
    }

    CMyComPtr<IGetFolderArcProps> getFolderArcProps;
    _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
    if (getFolderArcProps)
    {
      CMyComPtr<IFolderArcProps> getProps;
      getFolderArcProps->GetFolderArcProps(&getProps);
      if (getProps)
      {
        UInt32 numLevels;
        if (getProps->GetArcNumLevels(&numLevels) != S_OK)
          numLevels = 0;
        for (UInt32 level2 = 0; level2 < numLevels; level2++)
        {
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps(level, &numProps) == S_OK)
            {
              message += kSeparator;
              for (Int32 i = -3; i < (Int32)numProps; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                switch (i)
                {
                  case -3: propID = kpidPath; break;
                  case -2: propID = kpidType; break;
                  case -1: propID = kpidError; break;
                  default:
                    if (getProps->GetArcPropInfo(level, i, &name, &propID, &vt) != S_OK)
                      continue;
                }
                NCOM::CPropVariant prop;
                if (getProps->GetArcProp(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(propID, name, prop, message);
              }
            }
          }
          if (level2 != numLevels - 1)
          {
            UInt32 level = numLevels - 1 - level2;
            UInt32 numProps;
            if (getProps->GetArcNumProps2(level, &numProps) == S_OK)
            {
              message += kSeparatorSmall;
              for (Int32 i = 0; i < (Int32)numProps; i++)
              {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                if (getProps->GetArcPropInfo2(level, i, &name, &propID, &vt) != S_OK)
                  continue;
                NCOM::CPropVariant prop;
                if (getProps->GetArcProp2(level, propID, &prop) != S_OK)
                  continue;
                AddPropertyString(propID, name, prop, message);
              }
            }
          }
        }
      }
    }
    ::MessageBoxW(*(this), message, LangString(IDS_PROPERTIES, 0x03020900), MB_OK);
  }
}

void CPanel::EditCut()
{
  // InvokeSystemCommand("cut");
}

void CPanel::EditCopy()
{
  /*
  CMyComPtr<IGetFolderArcProps> getFolderArcProps;
  _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
  if (!getFolderArcProps)
  {
    InvokeSystemCommand("copy");
    return;
  }
  */
  UString s;
  CRecordVector<UInt32> indices;
  GetSelectedItemsIndices(indices);
  for (int i = 0; i < indices.Size(); i++)
  {
    if (i > 0)
      s += L"\xD\n";
    s += GetItemName(indices[i]);
  }
  ClipboardSetText(_mainWindow, s);
}

void CPanel::EditPaste()
{
  /*
  UStringVector names;
  ClipboardGetFileNames(names);
  CopyFromNoAsk(names);
  UString s;
  for (int i = 0; i < names.Size(); i++)
  {
    s += L" ";
    s += names[i];
  }

  MessageBoxW(0, s, L"", 0);
  */

  // InvokeSystemCommand("paste");
}

HRESULT CPanel::CreateShellContextMenu(
    const CRecordVector<UInt32> &operatedIndices,
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
      fileName += WCHAR_PATH_SEPARATOR;
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
      L"." WSTRING_PATH_SEPARATOR, &eaten, &pidl, 0);
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
    const CRecordVector<UInt32> &operatedIndices,
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
    const CRecordVector<UInt32> &operatedIndices,
    CMyComPtr<IContextMenu> &sevenZipContextMenu)
{
  sevenZipContextMenu.Release();

  CMenu menu;
  menu.Attach(menuSpec);
  // CMenuDestroyer menuDestroyer(menu);
  // menu.CreatePopup();

  bool sevenZipMenuCreated = false;

  CZipContextMenu *contextMenuSpec = new CZipContextMenu;
  CMyComPtr<IContextMenu> contextMenu = contextMenuSpec;
  // if (contextMenu.CoCreateInstance(CLSID_CZipContextMenu, IID_IContextMenu) == S_OK)
  {
    /*
    CMyComPtr<IInitContextMenu> initContextMenu;
    if (contextMenu.QueryInterface(IID_IInitContextMenu, &initContextMenu) != S_OK)
      return;
    */
    UString currentFolderUnicode = _currentFolderPrefix;
    UStringVector names;
    int i;
    for(i = 0; i < operatedIndices.Size(); i++)
      names.Add(currentFolderUnicode + GetItemRelPath(operatedIndices[i]));
    CRecordVector<const wchar_t *> namePointers;
    for(i = 0; i < operatedIndices.Size(); i++)
      namePointers.Add(names[i]);
    
    // NFile::NDirectory::MySetCurrentDirectory(currentFolderUnicode);
    if (contextMenuSpec->InitContextMenu(currentFolderUnicode, &namePointers.Front(),
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

  CRecordVector<UInt32> operatedIndices;
  GetOperatedItemIndices(operatedIndices);

  CMenu menu;
  menu.Attach(menuSpec);
  
  CreateSevenZipMenu(menu, operatedIndices, sevenZipContextMenu);
  if (g_App.ShowSystemMenu)
    CreateSystemMenu(menu, operatedIndices, systemContextMenu);

  /*
  if (menu.GetItemCount() > 0)
    menu.AppendItem(MF_SEPARATOR, 0, (LPCTSTR)0);
  */

  int i;
  for (i = 0; i < operatedIndices.Size(); i++)
    if (IsItemFolder(operatedIndices[i]))
      break;
  bool allAreFiles = (i == operatedIndices.Size());
  LoadFileMenu(menu, menu.GetItemCount(), programMenu,
      IsFSFolder(), operatedIndices.Size(), allAreFiles);
}

bool CPanel::InvokePluginCommand(int id)
{
  return InvokePluginCommand(id, _sevenZipContextMenu, _systemContextMenu);
}

bool CPanel::InvokePluginCommand(int id,
    IContextMenu *sevenZipContextMenu, IContextMenu *systemContextMenu)
{
  UInt32 offset;
  bool isSystemMenu = (id >= kSystemStartMenuID);
  if (isSystemMenu)
    offset = id  - kSystemStartMenuID;
  else
    offset = id  - kSevenZipStartMenuID;

  #ifdef UNDER_CE
  CMINVOKECOMMANDINFO
  #else
  CMINVOKECOMMANDINFOEX
  #endif
    commandInfo;
  commandInfo.cbSize = sizeof(commandInfo);
  commandInfo.fMask = 0
  #ifndef UNDER_CE
  | CMIC_MASK_UNICODE
  #endif
  ;
  commandInfo.hwnd = GetParent();
  commandInfo.lpVerb = (LPCSTR)(MAKEINTRESOURCE(offset));
  commandInfo.lpParameters = NULL;
  CSysString currentFolderSys = GetSystemString(_currentFolderPrefix);
  commandInfo.lpDirectory = (LPCSTR)(LPCTSTR)(currentFolderSys);
  commandInfo.nShow = SW_SHOW;
  commandInfo.lpParameters = NULL;
  #ifndef UNDER_CE
  commandInfo.lpTitle = "";
  commandInfo.lpVerbW = (LPCWSTR)(MAKEINTRESOURCEW(offset));
  UString currentFolderUnicode = _currentFolderPrefix;
  commandInfo.lpDirectoryW = currentFolderUnicode;
  commandInfo.lpTitleW = L"";
  // commandInfo.ptInvoke.x = xPos;
  // commandInfo.ptInvoke.y = yPos;
  commandInfo.ptInvoke.x = 0;
  commandInfo.ptInvoke.y = 0;
  #endif
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
  if (::GetParent((HWND)windowHandle) == _listView)
  {
    ShowColumnsContextMenu(xPos, yPos);
    return true;
  }

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

  CRecordVector<UInt32> operatedIndices;
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

  int result = menu.Track(TPM_LEFTALIGN
      #ifndef UNDER_CE
      | TPM_RIGHTBUTTON
      #endif
      | TPM_RETURNCMD | TPM_NONOTIFY,
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
