// PanelListNotify.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"

#include "../Common/PropIDUtils.h"
#include "../../PropID.h"

#include "Panel.h"
#include "FormatUtils.h"

using namespace NWindows;

/*
static UString ConvertSizeToStringShort(UInt64 value)
{
  wchar_t s[32];
  wchar_t c, c2 = L'B';
  if (value < (UInt64)10000)
  {
    c = L'B';
    c2 = L'\0';
  }
  else if (value < ((UInt64)10000 << 10))
  {
    value >>= 10;
    c = L'K';
  }
  else if (value < ((UInt64)10000 << 20))
  {
    value >>= 20;
    c = L'M';
  }
  else
  {
    value >>= 30;
    c = L'G';
  }
  ConvertUInt64ToString(value, s);
  int p = MyStringLen(s);
  s[p++] = L' ';
  s[p++] = c;
  s[p++] = c2;
  s[p++] = L'\0';
  return s;
}
*/

UString ConvertSizeToString(UInt64 value)
{
  wchar_t s[32];
  ConvertUInt64ToString(value, s);
  int i = MyStringLen(s);
  int pos = sizeof(s) / sizeof(s[0]);
  s[--pos] = L'\0';
  while (i > 3)
  {
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = s[--i];
    s[--pos] = L' ';
  }
  while (i > 0)
    s[--pos] = s[--i];
  return s + pos;
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
      NCOM::CPropVariant prop;
      _folder->GetProperty(index, kpidAttrib, &prop);
      UINT32 attrib = 0;
      if (prop.vt == VT_UI4)
        attrib = prop.ulVal;
      else if (IsItemFolder(index))
        attrib |= FILE_ATTRIBUTE_DIRECTORY;
      if (_currentFolderPrefix.IsEmpty())
        throw 1;
      else
        item.iImage = _extToIconMap.GetIconIndex(attrib, GetSystemString(GetItemName(index)));
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
    if (propID == kpidType)
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

  NCOM::CPropVariant prop;
  /*
  bool needRead = true;
  if (propID == kpidSize)
  {
    CComPtr<IFolderGetItemFullSize> getItemFullSize;
    if (_folder.QueryInterface(&getItemFullSize) == S_OK)
    {
      if (getItemFullSize->GetItemFullSize(index, &prop) == S_OK)
        needRead = false;
    }
  }
  if (needRead)
  */

  HRESULT res = _folder->GetProperty(realIndex, propID, &prop);
  if (res != S_OK)
    s = UString(L"Error: ") + HResultToMessage(res);
  else
  if ((prop.vt == VT_UI8 || prop.vt == VT_UI4) && (
      propID == kpidSize ||
      propID == kpidPackSize ||
      propID == kpidNumSubDirs ||
      propID == kpidNumSubFiles ||
      propID == kpidPosition ||
      propID == kpidNumBlocks ||
      propID == kpidClusterSize ||
      propID == kpidTotalSize ||
      propID == kpidFreeSpace
      ))
    s = ConvertSizeToString(ConvertPropVariantToUInt64(prop));
  else
  {
    s = ConvertPropertyToString(prop, propID, false);
    s.Replace(wchar_t(0xA), L' ');
    s.Replace(wchar_t(0xD), L' ');
  }
  int size = item.cchTextMax;
  if (size > 0)
  {
    if (s.Length() + 1 > size)
      s = s.Left(size - 1);
    MyStringCopy(item.pszText, (const wchar_t *)s);
  }
  return 0;
}

#ifndef UNDER_CE
extern DWORD g_ComCtl32Version;
#endif

void CPanel::OnItemChanged(NMLISTVIEW *item)
{
  int index = (int)item->lParam;
  if (index == kParentIndex)
    return;
  bool oldSelected = (item->uOldState & LVIS_SELECTED) != 0;
  bool newSelected = (item->uNewState & LVIS_SELECTED) != 0;
  // Don't change this code. It works only with such check
  if (oldSelected != newSelected)
    _selectedStatusVector[index] = newSelected;
}

extern bool g_LVN_ITEMACTIVATE_Support;

void CPanel::OnNotifyActivateItems()
{
  // bool leftCtrl = (::GetKeyState(VK_LCONTROL) & 0x8000) != 0;
  // bool rightCtrl = (::GetKeyState(VK_RCONTROL) & 0x8000) != 0;
  bool alt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
  bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
  bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
  if (!shift && alt && !ctrl)
    Properties();
  else
    OpenSelectedItems(!shift || alt || ctrl);
}

bool CPanel::OnNotifyList(LPNMHDR header, LRESULT &result)
{
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

      if ((dispInfo->item.mask & LVIF_TEXT) != 0 ||
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

    case LVN_ITEMACTIVATE:
      if (g_LVN_ITEMACTIVATE_Support)
      {
        OnNotifyActivateItems();
        return false;
      }
      break;
    case NM_DBLCLK:
    case NM_RETURN:
      if (!g_LVN_ITEMACTIVATE_Support)
      {
        OnNotifyActivateItems();
        return false;
      }
      break;

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
      if (_mySelectMode)
        #ifndef UNDER_CE
        if (g_ComCtl32Version >= MAKELONG(71, 4))
        #endif
          OnLeftClick((MY_NMLISTVIEW_NMITEMACTIVATE *)header);
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
    UInt64 totalSize = 0;
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
      NCOM::CPropVariant prop;
      if (_folder->GetProperty(realIndex, kpidMTime, &prop) == S_OK)
        dateString = ConvertPropertyToString(prop, kpidMTime, false);
    }
  }
  _statusBar.SetText(2, sizeString);
  _statusBar.SetText(3, dateString);
  // _statusBar.SetText(4, nameString);
  // _statusBar2.SetText(1, MyFormatNew(L"{0} bytes", NumberToStringW(totalSize)));
}
