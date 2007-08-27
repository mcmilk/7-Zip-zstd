// PanelListNotify.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
// #include "Windows/COM.h"

#include "../Common/PropIDUtils.h"
#include "../../PropID.h"

#include "Panel.h"
#include "FormatUtils.h"

using namespace NWindows;

static UString ConvertSizeToString(UINT64 value)
{
  wchar_t s[64];
  if (value < (UINT64(10000) <<  0) /*&& ((value & 0x3FF) != 0 || value == 0)*/)
  {
    ConvertUInt64ToString(value, s);
    return UString(s) + L" B";
  }
  if (value < (UINT64(10000) <<  10))
  {
    ConvertUInt64ToString((value >> 10), s);
    return UString(s) + L" K";
  }
  if (value < (UINT64(10000) <<  20))
  {
    ConvertUInt64ToString((value >> 20), s);
    return UString(s) + L" M";
  }
  ConvertUInt64ToString((value >> 30), s);
  return UString(s) + L" G";
}

LRESULT CPanel::SetItemText(LVITEMW &item)
{
  if (_dontShowMode)
    return 0;

  UINT32 realIndex = GetRealIndex(item);
  /*
  if ((item.mask & LVIF_IMAGE) != 0)
  {
    bool defined  = false;
    CComPtr<IFolderGetSystemIconIndex> folderGetSystemIconIndex;
    _folder.QueryInterface(&folderGetSystemIconIndex);
    if (folderGetSystemIconIndex)
    {
      folderGetSystemIconIndex->GetSystemIconIndex(index, &item.iImage);
      defined = (item.iImage > 0);
    }
    if (!defined)
    {
      NCOM::CPropVariant propVariant;
      _folder->GetProperty(index, kpidAttributes, &propVariant);
      UINT32 attributes = 0;
      if (propVariant.vt == VT_UI4)
        attributes = propVariant.ulVal;
      else
      {
        if (IsItemFolder(index))
          attributes |= FILE_ATTRIBUTE_DIRECTORY;
      }
      if (_currentFolderPrefix.IsEmpty())
      {
        throw 1;
      }
      else
        item.iImage = _extToIconMap.GetIconIndex(attributes, 
            GetSystemString(GetItemName(index)));
    }
    // item.iImage = 1;
  }
  */

  if ((item.mask & LVIF_TEXT) == 0)
    return 0;

  if (realIndex == kParentIndex)
    return 0;
  UString s;
  UINT32 subItemIndex = item.iSubItem;
  PROPID propID = _visibleProperties[subItemIndex].ID;
  /*
  {
    NCOM::CPropVariant property;
    if(propID == kpidType)
      string = GetFileType(index);
    else
    {
      HRESULT result = m_ArchiveFolder->GetProperty(index, propID, &property);
      if (result != S_OK)
      {
        // PrintMessage("GetPropertyValue error");
        return 0;
      }
      string = ConvertPropertyToString(property, propID, false);
    }
  }
  */
  // const NFind::CFileInfo &aFileInfo = m_Files[index];

  NCOM::CPropVariant propVariant;
  /*
  bool needRead = true;
  if (propID == kpidSize)
  {
    CComPtr<IFolderGetItemFullSize> getItemFullSize;
    if (_folder.QueryInterface(&getItemFullSize) == S_OK)
    {
      if (getItemFullSize->GetItemFullSize(index, &propVariant) == S_OK)
        needRead = false;
    }
  }
  if (needRead)
  */

  if (_folder->GetProperty(realIndex, propID, &propVariant) != S_OK)
      throw 2723407;

  if ((propID == kpidSize || propID == kpidPackedSize || 
      propID == kpidTotalSize || propID == kpidFreeSpace ||
      propID == kpidClusterSize)
      &&
      (propVariant.vt == VT_UI8 || propVariant.vt == VT_UI4))
    s = ConvertSizeToString(ConvertPropVariantToUInt64(propVariant));
  else
    s = ConvertPropertyToString(propVariant, propID, false);

  {
    s.Replace(wchar_t(0xA), L' '); 
    s.Replace(wchar_t(0xD), L' '); 
  }
  int size = item.cchTextMax;
  if(size > 0)
  {
    if(s.Length() + 1 > size)
      s = s.Left(size - 1);
    MyStringCopy(item.pszText, (const wchar_t *)s);
  }
  return 0;
}

extern DWORD g_ComCtl32Version;

void CPanel::OnItemChanged(NMLISTVIEW *item)
{
  int index = (int)item->lParam;
  if (index == kParentIndex)
    return;
  bool oldSelected = (item->uOldState & LVIS_SELECTED) != 0;
  bool newSelected = (item->uNewState & LVIS_SELECTED) != 0;
  // Don't change this code. It works only with such check
  if(oldSelected != newSelected)
    _selectedStatusVector[index] = newSelected;
}

bool CPanel::OnNotifyList(LPNMHDR header, LRESULT &result)
{
  // bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  // bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  // bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  switch(header->code)
  {
    case LVN_ITEMCHANGED:
    {
      if (_enableItemChangeNotify)
      {
        if (!_mySelectMode)
          OnItemChanged((LPNMLISTVIEW)header);
        RefreshStatusBar();
      }
      return false;
    }
    /*

    case LVN_ODSTATECHANGED:
      {
      break;
      }
    */

    case LVN_GETDISPINFOW:
    {
      LV_DISPINFOW *dispInfo = (LV_DISPINFOW *)header;

      //is the sub-item information being requested?

      if((dispInfo->item.mask & LVIF_TEXT) != 0 || 
        (dispInfo->item.mask & LVIF_IMAGE) != 0)
        SetItemText(dispInfo->item);
      return false;
    }
    case LVN_KEYDOWN:
    {
      bool boolResult = OnKeyDown(LPNMLVKEYDOWN(header), result);
      RefreshStatusBar();
      return boolResult;
    }

    case LVN_COLUMNCLICK:
      OnColumnClick(LPNMLISTVIEW(header));
      return false;
    /*
    case LVN_ITEMACTIVATE:
      RefreshStatusBar();
      if (!alt && !ctrl && !shift)
        OpenSelectedItems(true);
      return false;
    */

    case NM_DBLCLK:
      RefreshStatusBar();
      OpenSelectedItems(true);
      return false;
    case NM_RETURN:
    {
      bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
      bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
      // bool leftCtrl = (::GetKeyState(VK_LCONTROL) & 0x8000) != 0;
      // bool RightCtrl = (::GetKeyState(VK_RCONTROL) & 0x8000) != 0;
      bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
      if (!shift && alt && !ctrl)
      {
        Properties();
        return false;
      }
      OpenSelectedItems(true);
      return false;
    }
    case NM_RCLICK:
      RefreshStatusBar();
      break;

    /*
      return OnRightClick((LPNMITEMACTIVATE)header, result);
    */
      /*
      case NM_CLICK:
      SendRefreshStatusBarMessage();
      return 0;
      
        // TODO : Handler default action...
        return 0;
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
      */
    case NM_CLICK:
    {
      // we need SetFocusToList, if we drag-select items from other panel.
      SetFocusToList();
      RefreshStatusBar();
      if(_mySelectMode)
        if(g_ComCtl32Version >= MAKELONG(71, 4))
          OnLeftClick((LPNMITEMACTIVATE)header);
      return false;
    }
    case LVN_BEGINLABELEDITW:
      result = OnBeginLabelEdit((LV_DISPINFOW *)header);
      return true;
    case LVN_ENDLABELEDITW:
      result = OnEndLabelEdit((LV_DISPINFOW *)header);
      return true;

    case NM_CUSTOMDRAW:
    {
      if (_mySelectMode)
        return OnCustomDraw((LPNMLVCUSTOMDRAW)header, result);
      break;
    }
    case LVN_BEGINDRAG:
    {
      OnDrag((LPNMLISTVIEW)header);
      RefreshStatusBar();
      break;
    }
    // case LVN_BEGINRDRAG:
  }
  return false;
}

bool CPanel::OnCustomDraw(LPNMLVCUSTOMDRAW lplvcd, LRESULT &result)
{
  switch(lplvcd->nmcd.dwDrawStage) 
  {
  case CDDS_PREPAINT :
    result = CDRF_NOTIFYITEMDRAW;
    return true;
    
  case CDDS_ITEMPREPAINT:
    /*
    SelectObject(lplvcd->nmcd.hdc,
    GetFontForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam) );
    lplvcd->clrText = GetColorForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam);
    lplvcd->clrTextBk = GetBkColorForItem(lplvcd->nmcd.dwItemSpec,
    lplvcd->nmcd.lItemlParam);
    */
    int realIndex = (int)lplvcd->nmcd.lItemlParam;
    bool selected = false;
    if (realIndex != kParentIndex)
      selected = _selectedStatusVector[realIndex];
    if (selected)
      lplvcd->clrTextBk = RGB(255, 192, 192);
    // lplvcd->clrText = RGB(255, 0, 128);
    else
      lplvcd->clrTextBk = _listView.GetBkColor();
    // lplvcd->clrText = RGB(0, 0, 0);
    // result = CDRF_NEWFONT;
    result = CDRF_NOTIFYITEMDRAW;
    return true;
    
    // return false;
    // return true;
    /*
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
    if (lplvcd->iSubItem == 0)
    {
    // lplvcd->clrText = RGB(255, 0, 0);
    lplvcd->clrTextBk = RGB(192, 192, 192);
    }
    else
    {
    lplvcd->clrText = RGB(0, 0, 0);
    lplvcd->clrTextBk = RGB(255, 255, 255);
    }
    return true;
    */

        /* At this point, you can change the background colors for the item
        and any subitems and return CDRF_NEWFONT. If the list-view control
        is in report mode, you can simply return CDRF_NOTIFYSUBITEMREDRAW
        to customize the item's subitems individually */
  }
  return false;
}

void CPanel::OnRefreshStatusBar()
{
  CRecordVector<UINT32> indices;
  GetOperatedItemIndices(indices);

  _statusBar.SetText(0, MyFormatNew(IDS_N_SELECTED_ITEMS, 0x02000301, NumberToString(indices.Size())));

  UString selectSizeString;

  if (indices.Size() > 0)
  {
    UINT64 totalSize = 0;
    for (int i = 0; i < indices.Size(); i++)
      totalSize += GetItemSize(indices[i]);
    selectSizeString = ConvertSizeToString(totalSize);
  }
  _statusBar.SetText(1, selectSizeString);

  int focusedItem = _listView.GetFocusedItem();
  UString sizeString;
  UString dateString;
  if (focusedItem >= 0 && _listView.GetSelectedCount() > 0)
  {
    int realIndex = GetRealItemIndex(focusedItem);
    if (realIndex != kParentIndex)
    {
      sizeString = ConvertSizeToString(GetItemSize(realIndex));
      NCOM::CPropVariant propVariant;
      if (_folder->GetProperty(realIndex, kpidLastWriteTime, &propVariant) == S_OK)
        dateString = ConvertPropertyToString(propVariant, kpidLastWriteTime, false);
    }
  }
  _statusBar.SetText(2, sizeString);
  _statusBar.SetText(3, dateString);
  // _statusBar.SetText(4, nameString);
  // _statusBar2.SetText(1, MyFormatNew(L"{0} bytes", NumberToStringW(totalSize)));
}
