// ListViewDialog.cpp

#include "StdAfx.h"
#include "ListViewDialog.h"

#ifdef LANG        
#include "../../LangUtils.h"
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
    LVITEM anItem;
    anItem.mask = LVIF_TEXT;
    anItem.iItem = i;
    anItem.pszText = (LPTSTR)(LPCTSTR)Strings[i];
    anItem.iSubItem = 0;
    _listView.InsertItem(&anItem);
  }
  if (Strings.Size() > 0)
    _listView.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

  return CModalDialog::OnInit();
}

void CListViewDialog::OnOK()
{
  ItemIndex = _listView.GetFocusedItem();
  CModalDialog::OnOK();
}
