// MessagesDialog.cpp
 
#include "StdAfx.h"
#include "MessagesDialog.h"
#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../../LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000713 }
};
#endif

void CMessagesDialog::AddMessageDirect(LPCTSTR message)
{
  int itemIndex = _messageList.GetItemCount();
  LVITEM item;
  item.mask = LVIF_TEXT;
  item.iItem = itemIndex;

  CSysString stringNumber;
  TCHAR sz[32];
  wsprintf(sz, TEXT("%d"), itemIndex);
  stringNumber = sz;

  item.pszText = (LPTSTR)(LPCTSTR)stringNumber;
  item.iSubItem = 0;
  _messageList.InsertItem(&item);

  item.mask = LVIF_TEXT;
  item.pszText = (LPTSTR)message;
  item.iSubItem = 1;
  _messageList.SetItem(&item);
}

void CMessagesDialog::AddMessage(LPCTSTR message)
{
  CSysString s = message;
  while (!s.IsEmpty())
  {
    int pos = s.Find(TEXT('\n'));
    if (pos < 0)
      break;
    AddMessageDirect(s.Left(pos));
    s.Delete(0, pos + 1);
  }
  AddMessageDirect(s);
}

bool CMessagesDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000A00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _messageList.Attach(GetItem(IDC_MESSAGE_LIST));

  LVCOLUMN columnInfo;
  columnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  columnInfo.fmt = LVCFMT_LEFT;
  columnInfo.pszText = TEXT("#");
  columnInfo.iSubItem = 0;
  columnInfo.cx = 30;

  _messageList.InsertColumn(0, &columnInfo);


  columnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  columnInfo.fmt = LVCFMT_LEFT;
  #ifdef LANG
  CSysString s = LangLoadString(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN, 0x02000A80);
  #else
  CSysString s = MyLoadString(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN);
  #endif

  columnInfo.pszText = (LPTSTR)(LPCTSTR)s;
  columnInfo.iSubItem = 1;
  columnInfo.cx = 450;

  _messageList.InsertColumn(1, &columnInfo);

  for(int i = 0; i < Messages->Size(); i++)
    AddMessage((*Messages)[i]);

  /*
  if(_messageList.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    _messageList.SetItemState(0, aState, aState);
  }
  */
  return CModalDialog::OnInit();
}
