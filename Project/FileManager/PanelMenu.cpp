#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Windows/Menu.h"

#include "Panel.h"
#include "PluginInterface.h"
#include "MyLoadMenu.h"

using namespace NWindows;

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

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
  GetOperatedItemIndexes(operatedIndices);

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

  CComPtr<IContextMenu> contextMenu;
  const kPluginMenuStartID = 1000;
  bool sevenZipMenuCreated = false;
  UString currentFolderUnicode;
  CSysString currentFolderSys;

  if (contextMenu.CoCreateInstance(CLSID_CZipContextMenu) == S_OK)
  {
    CComPtr<IInitContextMenu> initContextMenu;
    if (contextMenu.QueryInterface(&initContextMenu) != S_OK)
      return true;
    currentFolderUnicode = GetUnicodeString(_currentFolderPrefix);;
    currentFolderSys = GetSystemString(_currentFolderPrefix);
    UString folder = GetUnicodeString(_currentFolderPrefix);
    UStringVector names;
    for(int i = 0; i < operatedIndices.Size(); i++)
      names.Add(currentFolderUnicode + GetItemName(operatedIndices[i]));
    CRecordVector<const wchar_t *> namePointers;
    for(i = 0; i < operatedIndices.Size(); i++)
      namePointers.Add(names[i]);
    
    SetCurrentDirectory(GetSystemString(_currentFolderPrefix));
    if (initContextMenu->InitContextMenu(folder, &namePointers.Front(),
        operatedIndices.Size()) == S_OK)
    {
      HRESULT res = contextMenu->QueryContextMenu(menu, 0, kPluginMenuStartID, 
        kPluginMenuStartID + 100, 0);
      sevenZipMenuCreated = (HRESULT_SEVERITY(res) == SEVERITY_SUCCESS);
      // int code = HRESULT_CODE(res);
      // int nextItemID = code;
    }
  }
  if (sevenZipMenuCreated)
    menu.AppendItem(MF_SEPARATOR, 0, 0);
  LoadFileMenu(menu, menu.GetItemCount(), !operatedIndices.IsEmpty());

  /*
  if (numSelectedItems == 1)
    menu.AppendItem(MF_STRING, kMenuIDRename, TEXT("Rename"));
  if (numSelectedItems > 0)
  {
    menu.AppendItem(MF_STRING, kMenuIDCopy, TEXT("Copy"));
    menu.AppendItem(MF_STRING, kMenuIDMove, TEXT("Move"));
    menu.AppendItem(MF_STRING, kMenuIDDelete, TEXT("Delete"));
  }
  */
  
  int result = menu.Track(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, 
    xPos, yPos, _listView);

  if (result == 0)
    return true;

  if (ExecuteFileCommand(result))
    return true;


  if (!sevenZipMenuCreated)
    return true;

  UINT32 offset = result - kPluginMenuStartID;

  /*
  const kVerbSize = 300;
  WCHAR verb[kVerbSize];
  if (contextMenu->GetCommandString(offset, GCS_VERBW, NULL, 
      LPSTR(verb), kVerbSize) != NOERROR)
    return true;
  */

  CMINVOKECOMMANDINFOEX commandInfo;
  commandInfo.cbSize = sizeof(commandInfo);
  commandInfo.fMask = CMIC_MASK_UNICODE;
  commandInfo.hwnd = GetParent();
  commandInfo.lpVerb = LPCSTR(offset);
  commandInfo.lpParameters = NULL;
  commandInfo.lpDirectory = (LPCSTR)(LPCTSTR)(currentFolderSys);
  commandInfo.nShow = SW_SHOW;
  commandInfo.lpTitle = "";
  commandInfo.lpVerbW = LPCWSTR(offset);
  commandInfo.lpParameters = NULL;
  commandInfo.lpDirectoryW = currentFolderUnicode;
  commandInfo.lpTitleW = L"";
  commandInfo.ptInvoke.x = xPos;
  commandInfo.ptInvoke.y = yPos;

  if (contextMenu->InvokeCommand(LPCMINVOKECOMMANDINFO(&commandInfo)) == NOERROR)
  {
    KillSelection();
  }
  else
  {
  }
 
  return true;
}
