// PanelKey.cpp

#include "StdAfx.h"

#include "Panel.h"
#include "HelpUtils.h"

#include "Interface/PropID.h"

// static LPCTSTR kFoldersTopic = _T("FM/index.htm");


struct CVKeyPropIDPair
{
  WORD VKey;
  PROPID PropID;
};

static CVKeyPropIDPair g_VKeyPropIDPairs[] = 
{
  { VK_F3, kpidName },
  { VK_F4, kpidExtension },
  { VK_F5, kpidLastWriteTime },
  { VK_F6, kpidSize },
  { VK_F7, kpidNoProperty }
};

static int FindVKeyPropIDPair(WORD vKey)
{
  for (int i = 0; i < sizeof(g_VKeyPropIDPairs) / sizeof(g_VKeyPropIDPairs[0]); i++)
    if (g_VKeyPropIDPairs[i].VKey == vKey)
      return i;
  return -1;
}


bool CPanel::OnKeyDown(LPNMLVKEYDOWN keyDownInfo, LRESULT &result)
{
  if (keyDownInfo->wVKey == VK_TAB && keyDownInfo->hdr.hwndFrom == _listView)
  {
    _panelCallback->OnTab();
    return false;
  }
  bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  bool leftCtrl = (::GetKeyState(VK_LCONTROL) & 0x8000) != 0;
  bool RightCtrl = (::GetKeyState(VK_RCONTROL) & 0x8000) != 0;
  bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  result = 0;

  if (keyDownInfo->wVKey >= '0' && keyDownInfo->wVKey <= '9' && RightCtrl)
  {
    int index = keyDownInfo->wVKey - '0';
    if (shift)
    {
      FastFolderInsert(index);
      return true;
    }
    else
    {
      FastFolderSelect(index);
      return true;
    }
  }

  if ((keyDownInfo->wVKey == VK_F2 || 
    keyDownInfo->wVKey == VK_F1) && alt && !ctrl && !shift)
  {
    _panelCallback->OnSetFocusToPath(keyDownInfo->wVKey == VK_F1 ? 0 : 1);
    return true;
  }

  if(keyDownInfo->wVKey >= VK_F3 && keyDownInfo->wVKey <= VK_F12 && ctrl)
  {
    int index = FindVKeyPropIDPair(keyDownInfo->wVKey);
    if (index >= 0)
      SortItemsWithPropID(g_VKeyPropIDPairs[index].PropID);
  }

  switch(keyDownInfo->wVKey)
  {
    case VK_SHIFT:
    {
      _selectionIsDefined = false;
      _prevFocusedItem = _listView.GetFocusedItem();
      break;
    }
    /*
    case VK_F1:
    {
      // ShowHelpWindow(NULL, kFoldersTopic);
      break;
    }
    */
    case VK_F2:
    {
      if (!alt && !ctrl &&!shift)
      {
        RenameFile();
        return true;
      }
      break;
    }
    case VK_F4:
    {
      if (!alt && !ctrl && !shift)
      {
        EditItem();
        return true;
      }
      if (!alt && !ctrl && shift)
      {
        CreateFile();
        return true;
      }
      break;
    }
    case VK_F5:
    {
      if (!alt && !ctrl)
      {
        _panelCallback->OnCopy(false, shift);
        return true;
      }
      break;
    }
    case VK_F6:
    {
      if (!alt && !ctrl)
      {
        _panelCallback->OnCopy(true, shift);
        return true;
      }
      break;
    }
    /*
    case VK_F7:
    {
      if (!alt && !ctrl && !shift)
      {
        CreateFolder();
        return true;
      }
      break;
    }
    */
    case VK_DELETE:
    {
      // if (shift)
      DeleteItems();
      return true;
    }
    case VK_INSERT:
    {
      OnInsert();
      return true;
    }
    case VK_DOWN:
    {
      if(shift)
        OnArrowWithShift();
      return false;
    }
    case VK_UP:
    {
      if (alt)
        _panelCallback->OnSetSameFolder();
      else if(shift)
        OnArrowWithShift();
      return false;
    }
    case VK_RIGHT:
    {
      if (alt)
        _panelCallback->OnSetSubFolder();
      else if(shift)
        OnArrowWithShift();
      return false;
    }
    case VK_LEFT:
    {
      if (alt)
        _panelCallback->OnSetSubFolder();
      else if(shift)
        OnArrowWithShift();
      return false;
    }
    case VK_NEXT:
    {
      if (ctrl && !alt && !shift)
      {
        // EnterToFocused();
        return true;
      }
      break;
    }
    case VK_ADD:
    {
      if (alt)
        SelectByType(true);
      else if (shift)
        SelectAll(true);
      else if(!ctrl)
        SelectSpec(true);
      return true;
    }
    case VK_SUBTRACT:
    {
      if (alt)
        SelectByType(false);
      else if (shift)
        SelectAll(false);
      else
        SelectSpec(false);
      return true;
    }
    /*
    case VK_DELETE:
      CommandDelete();
      return 0;
    case VK_F1:
      CommandHelp();
      return 0;
    */
    case VK_BACK:
      OpenParentFolder();
      return true;
    /*
    case VK_DIVIDE:
    case '\\':
    case '/':
    case VK_OEM_5:
    {
      // OpenRootFolder();
      OpenDrivesFolder();

      return true;
    }
    */
    case 'A':
      if(ctrl)
      {
        int numItems = _listView.GetItemCount();
        for (int i = 0; i < numItems; i++)
          _selectedStatusVector[i] = true;
        if (numItems > 0)
          _listView.RedrawItems(0, numItems - 1);

        // m_RedrawEnabled = false;
        // _listView.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
        // m_RedrawEnabled = true;
        // ShowStatusBar();
        return true;

      }
      return false;
    case 'N':
      if (ctrl)
      {
        CreateFile();
        return true;
      }
      return false;
    case 'R':
      if(ctrl)
      {
        OnReload();
        return true;
      }
      return false;
    case '1':
    case '2':
    case '3':
    case '4':
      if(ctrl)
      {
        int styleIndex = keyDownInfo->wVKey - '1';
        SetListViewMode(styleIndex);
        return true;
      }
      return false;
    case VK_MULTIPLY:
      {
        InvertSelection();
        return true;
      }
    case VK_F12:
      if (alt && !ctrl && !shift)
      {
        FoldersHistory();
        return true;
      }
  }
  return false;
}
