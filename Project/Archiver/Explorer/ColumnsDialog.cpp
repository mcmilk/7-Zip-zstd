// ColumnsDialog.cpp

#include "StdAfx.h"
#include "resource.h"

#include "ColumnsDialog.h"

#include "Windows/Defs.h"
#include "Windows/ResourceString.h"

#include "../Common/HelpUtils.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_STATIC_COLUMNS_HEADER, 0x02000E01 },
  { IDC_STATIC_COLUMNS_WIDTH_BEGIN, 0x02000E02 },
  { IDC_STATIC_COLUMNS_WIDTH_END, 0x02000E03 },
  { IDC_COLUMNS_BUTTON_MOVE_UP, 0x02000E10 },
  { IDC_COLUMNS_BUTTON_MOVE_DOWN, 0x02000E11 },
  { IDC_COLUMNS_BUTTON_SHOW, 0x02000E12 },
  { IDC_COLUMNS_BUTTON_HIDE, 0x02000E13 },
  { IDC_COLUMNS_BUTTON_SET_WIDTH, 0x02000E14 },
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 },
  { IDHELP, 0x02000720 }
};
#endif


static bool LessFunc(const NColumnsDialog::CColumnInfo &a1, const NColumnsDialog::CColumnInfo &a2)
{
  if(a1.IsVisible && !a2.IsVisible)
    return true;
  if(!a1.IsVisible && a2.IsVisible)
    return false;
  return a1.Order < a2.Order;
}

static CSysString GetNumber(int aValue)
{
  TCHAR aChars[32];
  _stprintf(aChars, _T("%d"), aValue);
  return aChars;
}

void CColumnsDialog::SetWidthOfItem(int anIndex)
{
  const NColumnsDialog::CColumnInfo &aColumnInfo = m_ColumnInfoVector[anIndex];
  LVITEM anItem;
  anItem.iItem = anIndex;
  anItem.mask = LVIF_TEXT;
  CSysString aNumberString = GetNumber(aColumnInfo.Width);
  anItem.pszText = (LPTSTR)(LPCTSTR)aNumberString;
  anItem.iSubItem = 1;
  m_ListView.SetItem(&anItem);
}

void CColumnsDialog::SetItem(int anIndex)
{
  const NColumnsDialog::CColumnInfo &aColumnInfo = m_ColumnInfoVector[anIndex];
  LVITEM anItem;
  anItem.iItem = anIndex;
  
  //anItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
  anItem.mask = LVIF_TEXT | LVIF_STATE;
  anItem.pszText = (TCHAR *)(const TCHAR *)aColumnInfo.Caption;
  anItem.iSubItem = 0;
  // anItem.lParam = anIndex;
  anItem.state = 0;
  anItem.stateMask = UINT(-1);
  m_ListView.SetItem(&anItem);
  m_ListView.SetCheckState(anItem.iItem, aColumnInfo.IsVisible);
  
  SetWidthOfItem(anIndex);
}

void CColumnsDialog::AddRows()
{
  m_ListView.SetRedraw(false);
  m_ListView.DeleteAllItems();
  for(int i = 0; i < m_ColumnInfoVector.size(); i++)
  {
    LVITEM anItem;
    anItem.iItem = i;
    anItem.mask = 0;
    anItem.iSubItem = 0;
    m_ListView.InsertItem(&anItem);
    SetItem(i);
  }
  m_ListView.SetRedraw(true);
}

/*
static void MyEnableWindow(CWnd &aWindow, bool aNewState)
{
  bool anOldState = BOOLToBool(aWindow.IsWindowEnabled());
  if(anOldState != aNewState)
    aWindow.EnableWindow(BoolToBOOL(aNewState));
}
*/

void CColumnsDialog::RefreshButtons() 
{
  if (!m_RefreshButtonsEnabled)
    return;
  int aNumSelectedItems = m_ListView.GetSelectedCount();
  if(aNumSelectedItems > 1 || aNumSelectedItems == 0)
  {
    m_ButtonMoveUp.Enable(false);
    m_ButtonMoveDown.Enable(false);
  }
  bool aThereIsUnChecked = false;
  bool aThereIsChecked = false;
  
  bool anAllWidthsAreEqual = true;
  int aWidth = -1;

  int anIndex = -1;
  for(int i = 0; true; i++)
  {
    anIndex = m_ListView.GetNextSelectedItem(anIndex);
    if (anIndex == -1)
      break;
    if(i == 0 && aNumSelectedItems == 1)
    {
      m_ButtonMoveUp.Enable(anIndex > 0);
      m_ButtonMoveDown.Enable(anIndex < m_ListView.GetItemCount() - 1);
    }
    bool aChecked = m_ListView.GetCheckState(anIndex);
    aThereIsChecked |= aChecked;
    aThereIsUnChecked |= !aChecked;

    if(anAllWidthsAreEqual)
    {
      int aNewWidth = m_ColumnInfoVector[anIndex].Width;
      if(aWidth < 0)
        aWidth = aNewWidth;
      else 
        anAllWidthsAreEqual = (aWidth == aNewWidth);
    }
  }
  CSysString aWidthString;
  if(anAllWidthsAreEqual && aWidth >= 0)
    aWidthString = GetNumber(aWidth);
  m_Width.SetText(aWidthString);
  m_ButtonHide.Enable(aThereIsChecked);
  m_ButtonShow.Enable(aThereIsUnChecked);
}

bool CColumnsDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x02000E00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  m_Width.Init(*this, IDC_COLUMN_EDIT_WIDTH);
  m_Width.SetText(_T(""));
  m_ListView.Attach(GetItem(IDC_COLUMNS_LISTVIEW));

  m_ButtonMoveDown.Attach(GetItem(IDC_COLUMNS_BUTTON_MOVE_DOWN));
  m_ButtonMoveUp.Attach(GetItem(IDC_COLUMNS_BUTTON_MOVE_UP));
  m_ButtonShow.Attach(GetItem(IDC_COLUMNS_BUTTON_SHOW));
  m_ButtonHide.Attach(GetItem(IDC_COLUMNS_BUTTON_HIDE));

  // 4.70 
  CRefreshButtons aRefreshButtons(m_RefreshButtonsEnabled);


  UINT32 aNewFlags = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
  m_ListView.SetExtendedListViewStyle(aNewFlags, aNewFlags);

  CSysString aString = LangLoadString(IDS_COLUMN_TITLE, 0x02000E81);
  LVCOLUMN aColumn;
  aColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  aColumn.cx = 160;
  aColumn.fmt = LVCFMT_LEFT;
  aColumn.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumn.iSubItem = 0;
  m_ListView.InsertColumn(0, &aColumn);

  aString = LangLoadString(IDS_COLUMN_WIDTH, 0x02000E82);
  aColumn.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT | LVCF_SUBITEM;
  aColumn.cx = 80;
  aColumn.fmt = LVCFMT_RIGHT;
  aColumn.pszText = (LPTSTR)(LPCTSTR)aString;
  aColumn.iSubItem = 1;
  m_ListView.InsertColumn(1, &aColumn);

  std::sort(m_ColumnInfoVector.begin(), m_ColumnInfoVector.end(), LessFunc);
  AddRows();
  if(m_ListView.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    m_ListView.SetItemState(0, aState, aState);
  }
  aRefreshButtons.Enable();
  RefreshButtons();
  return CModalDialog::OnInit();
}


void CColumnsDialog::SwapItems(int anIndexOld, int anIndexNew) 
{
  std::swap(m_ColumnInfoVector[anIndexOld].Order, m_ColumnInfoVector[anIndexNew].Order);
  std::swap(m_ColumnInfoVector[anIndexOld], m_ColumnInfoVector[anIndexNew]);
  m_ListView.SetRedraw(false);
  m_ListView.SetFocus();
  SetItem(anIndexOld);
  SetItem(anIndexNew);
  if(m_ListView.GetItemCount() > 0)
  {
    UINT aState = LVIS_SELECTED | LVIS_FOCUSED;
    m_ListView.SetItemState(anIndexNew, aState, aState);
    m_ListView.EnsureVisible(anIndexNew, false);
  }
  m_ListView.SetRedraw(true);
}

bool CColumnsDialog::OnButtonClicked(int aButtonID, HWND aButtonHWND)
{
  switch(aButtonID)
  {
    case IDC_COLUMNS_BUTTON_MOVE_UP:
      OnButtonMoveUp();
      break;
    case IDC_COLUMNS_BUTTON_MOVE_DOWN:
      OnButtonMoveDown();
      break;
    case IDC_COLUMNS_BUTTON_HIDE:
      OnButtonShow(false);
      break;
    case IDC_COLUMNS_BUTTON_SHOW:
      OnButtonShow(true);
      break;
    case IDC_COLUMNS_BUTTON_SET_WIDTH:
      OnButtonSetWidth();
      break;
    default:
      return CModalDialog::OnButtonClicked(aButtonID, aButtonHWND);
  }
  return true;
}

void CColumnsDialog::OnButtonMoveUp() 
{
  CRefreshButtons aRefreshButtons(m_RefreshButtonsEnabled);
  if(m_ListView.GetSelectedCount() != 1)
    return;
  int anIndex = m_ListView.GetNextSelectedItem(-1);
  if (anIndex > 0)
    SwapItems(anIndex, anIndex  - 1);
  aRefreshButtons.Enable();
  RefreshButtons();
}

void CColumnsDialog::OnButtonMoveDown() 
{
  CRefreshButtons aRefreshButtons(m_RefreshButtonsEnabled);
  if(m_ListView.GetSelectedCount() != 1)
    return;
  int anIndex = m_ListView.GetNextSelectedItem(-1);
  if (anIndex < m_ListView.GetItemCount() - 1)
    SwapItems(anIndex, anIndex  + 1);
  aRefreshButtons.Enable();
  RefreshButtons();
}

void CColumnsDialog::OnButtonShow(bool aState) 
{
  CRefreshButtons aRefreshButtons(m_RefreshButtonsEnabled);
  int aPos = -1;
  while (true)
  {
    aPos = m_ListView.GetNextSelectedItem(aPos);
    if (aPos < 0)
      break;
    m_ListView.SetCheckState(aPos, aState);
  }
  m_ListView.SetFocus();
  aRefreshButtons.Enable();
  RefreshButtons();
}

bool CColumnsDialog::OnNotify(UINT aControlID, LPNMHDR lParam) 
{ 
  if (lParam->hwndFrom == m_ListView && lParam->code == LVN_ITEMCHANGED)
  {
    OnItemchangedColumnsListview((const NMLISTVIEW *)lParam);
    return true;
  }
  return CModalDialog::OnNotify(aControlID, lParam); 
}

void CColumnsDialog::OnItemchangedColumnsListview(const NMLISTVIEW *pNMListView) 
{
  if (!m_RefreshButtonsEnabled)
    return;
  int anIndex = pNMListView->iItem;
  bool aStatus = 
  m_ColumnInfoVector[anIndex].IsVisible = m_ListView.GetCheckState(anIndex);
	RefreshButtons();
}

void CColumnsDialog::OnButtonSetWidth() 
{
  CSysString aWidth;
  m_Width.GetText(aWidth);

  aWidth.TrimLeft();
  aWidth.TrimRight();
  TCHAR *anEndPtr;
  UINT aNewWidth = _tcstoul(aWidth, &anEndPtr, 10);
  CRefreshButtons aRefreshButtons(m_RefreshButtonsEnabled);
  int anIndex = -1;
  while (true)
  {
    anIndex = m_ListView.GetNextSelectedItem(anIndex);
    if (anIndex < 0)
      break;
    m_ColumnInfoVector[anIndex].Width = aNewWidth;
    SetWidthOfItem(anIndex);
  }
  aRefreshButtons.Enable();
  RefreshButtons();
}

void CColumnsDialog::OnOK() 
{
  int aNumItems = m_ListView.GetItemCount();
  for(int i = 0; i < aNumItems; i++)
  {
    NColumnsDialog::CColumnInfo aColumnInfo;
    // int anIndex = m_ListView.GetItemData(i);
    // m_ColumnInfoVector[anIndex].IsVisible = BOOLToBool(m_ListView.GetCheck(i));
    m_ColumnInfoVector[i].IsVisible = m_ListView.GetCheckState(i);
  }
  // sort(m_ColumnInfoVector.begin(), m_ColumnInfoVector.end(), LessFunc);
	CModalDialog::OnOK();
}

static LPCTSTR kHelpTopic = _T("gui/columns.htm");

void CColumnsDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
}
