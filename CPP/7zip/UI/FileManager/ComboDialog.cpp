// ComboDialog.cpp

#include "StdAfx.h"
#include "ComboDialog.h"

#include "Windows/Control/Static.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDOK, 0x02000702 },
  { IDCANCEL, 0x02000710 }
};
#endif

bool CComboDialog::OnInit() 
{
  #ifdef LANG        
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif
  _comboBox.Attach(GetItem(IDC_COMBO_COMBO));

  /*
  // why it doesn't work ?
  DWORD style = _comboBox.GetStyle();
  if (Sorted)
    style |= CBS_SORT;
  else
    style &= ~CBS_SORT;
  _comboBox.SetStyle(style);
  */
  SetText(Title);
  
  NControl::CStatic staticContol;
  staticContol.Attach(GetItem(IDC_COMBO_STATIC));
  staticContol.SetText(Static);
  _comboBox.SetText(Value);
  for(int i = 0; i < Strings.Size(); i++)
    _comboBox.AddString(Strings[i]);
  return CModalDialog::OnInit();
}

void CComboDialog::OnOK()
{
  _comboBox.GetText(Value);
  CModalDialog::OnOK();
}
