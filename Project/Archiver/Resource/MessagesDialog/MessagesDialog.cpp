// MessagesDialog.cpp
 
#include "StdAfx.h"
#include "MessagesDialog.h"
#include "Windows/ResourceString.h"

// #include "../resource.h"

#ifdef LANG        
#include "../../Common/LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000713 }
};
#endif

void CMessagesDialog::AddMessage(LPCTSTR aMessage)
{
  int anItemIndex = m_MessageList.GetItemCount();
  LVITEM anItem;
  anItem.mask = LVIF_TEXT;
  anItem.iItem = anItemIndex;

  CSysString aStringNumber;
  TCHAR sz[32];
  _stprintf(sz, _T("%d"), anItemIndex);
  aStringNumber = sz;

  anItem.pszText = (LPTSTR)(LPCTSTR)aStringNumber;
  anItem.iSubItem = 0;
  m_MessageList.InsertItem(&anItem);

  anItem.mask = LVIF_TEXT;
  anItem.pszText = (LPTSTR)aMessage;
  anItem.iSubItem = 1;
  m_MessageList.SetItem(&anItem);
}

bool CMessagesDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000A00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  m_MessageList.Attach(GetItem(IDC_MESSAGE_LIST));

  LVCOLUMN aColumnInfo;
  aColumnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  aColumnInfo.fmt = LVCFMT_LEFT;
  aColumnInfo.pszText = _T("#");
  aColumnInfo.iSubItem = 0;
  aColumnInfo.cx = 30;

  m_MessageList.InsertColumn(0, &aColumnInfo);


  aColumnInfo.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  aColumnInfo.fmt = LVCFMT_LEFT;
  #ifdef LANG
  CSysString aString = LangLoadString(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN, 0x02000A80);
  #else
  CSysString aString = MyLoadString(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN);
  #endif

  aColumnInfo.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumnInfo.iSubItem = 1;
  aColumnInfo.cx = 450;

  m_MessageList.InsertColumn(1, &aColumnInfo);

  for(int i = 0; i < m_Messages->Size(); i++)
    AddMessage((*m_Messages)[i]);

  /*
  if(m_MessageList.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    m_MessageList.SetItemState(0, aState, aState);
  }
  */
	return CModalDialog::OnInit();
}
