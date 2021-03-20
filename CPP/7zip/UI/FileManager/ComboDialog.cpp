// ComboDialog.cpp

#include "StdAfx.h"
#include "ComboDialog.h"

#include "../../../Windows/Control/Static.h"

#ifdef LANG
#include "LangUtils.h"
#endif

using namespace NWindows;

bool CComboDialog::OnInit()
{
  #ifdef LANG
  LangSetDlgItems(*this, NULL, 0);
  #endif
  _comboBox.Attach(GetItem(IDC_COMBO));

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
  staticContol.Attach(GetItem(IDT_COMBO));
  staticContol.SetText(Static);
  _comboBox.SetText(Value);
  FOR_VECTOR (i, Strings)
    _comboBox.AddString(Strings[i]);
  NormalizeSize();
  return CModalDialog::OnInit();
}

void CComboDialog::OnOK()
{
  _comboBox.GetText(Value);
  CModalDialog::OnOK();
}
