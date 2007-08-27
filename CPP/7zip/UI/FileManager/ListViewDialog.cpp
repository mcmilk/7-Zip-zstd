// ListViewDialog.cpp

#include "StdAfx.h"
#include "ListViewDialog.h"

#ifdef LANG        
#include "LangUtils.h"
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif

bool CListViewDialog::OnInit() 
{
  #ifdef LANG        
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _listView.Attach(GetItem(IDC_LISTVIEW_LIST));
  SetText(Title);

  LVCOLUMN columnInfo;
  columnInfo.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
  columnInfo.fmt = LVCFMT_LEFT;
  columnInfo.iSubItem = 0;
  columnInfo.cx = 1000;

  _listView.InsertColumn(0, &columnInfo);

  for(int i = 0; i < Strings.Size(); i++)
  {
    LVITEMW item;
    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.pszText = (LPWSTR)(LPCWSTR)Strings[i];
    item.iSubItem = 0;
    _listView.InsertItem(&item);
  }
  if (Strings.Size() > 0)
    _listView.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
  StringsWereChanged = false;
  return CModalDialog::OnInit();
}

bool CListViewDialog::OnNotify(UINT /* controlID */, LPNMHDR header)
{
  if (header->hwndFrom != _listView)
    return false;
  switch(header->code)
  {
    case LVN_KEYDOWN:
    {
      LPNMLVKEYDOWN keyDownInfo = LPNMLVKEYDOWN(header);
      switch(keyDownInfo->wVKey)
      {
        case VK_DELETE:
        {
          if (!DeleteIsAllowed)
            return false;
          int focusedIndex = _listView.GetFocusedItem();
          if (focusedIndex < 0)
            focusedIndex = 0;
          for (;;)
          {
            int index = _listView.GetNextSelectedItem(-1);
            if (index < 0)
              break;
            StringsWereChanged = true;
            _listView.DeleteItem(index);
            Strings.Delete(index);
          }
          if (focusedIndex >= _listView.GetItemCount())
            focusedIndex = _listView.GetItemCount() - 1;
          if (focusedIndex >= 0)
            _listView.SetItemState(focusedIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
          return true;
        }
        case 'A':
        {
          bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
          if (ctrl)
          {
            int numItems = _listView.GetItemCount();
            for (int i = 0; i < numItems; i++)
              _listView.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
            return true;
          }
        }
      }
    }
  }
  return false;
}

void CListViewDialog::OnOK()
{
  FocusedItemIndex = _listView.GetFocusedItem();
  CModalDialog::OnOK();
}
