// MessagesDialog.cpp
 
#include "StdAfx.h"
#include "MessagesDialog.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Windows/ResourceString.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000713 }
};
#endif

void CMessagesDialog::AddMessageDirect(LPCWSTR message)
{
  int itemIndex = _messageList.GetItemCount();
  LVITEMW item;
  item.mask = LVIF_TEXT;
  item.iItem = itemIndex;

  wchar_t sz[32];
  ConvertInt64ToString(itemIndex, sz);

  item.pszText = sz;
  item.iSubItem = 0;
  _messageList.InsertItem(&item);

  item.pszText = (LPWSTR)message;
  item.iSubItem = 1;
  _messageList.SetItem(&item);
}

void CMessagesDialog::AddMessage(LPCWSTR message)
{
  UString s = message;
  while (!s.IsEmpty())
  {
    int pos = s.Find(L'\n');
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
  _messageList.SetUnicodeFormat(true);

  LVCOLUMNW columnInfo;
  columnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  columnInfo.fmt = LVCFMT_LEFT;
  columnInfo.pszText = L"#";
  columnInfo.iSubItem = 0;
  columnInfo.cx = 30;

  _messageList.InsertColumn(0, &columnInfo);


  columnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  columnInfo.fmt = LVCFMT_LEFT;
  UString s = 
  #ifdef LANG        
  LangString(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN, 0x02000A80);
  #else
  MyLoadStringW(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN);
  #endif

  columnInfo.pszText = (LPWSTR)(LPCWSTR)s;
  columnInfo.iSubItem = 1;
  columnInfo.cx = 600;

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
