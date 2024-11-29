// LangPage.h
 
#ifndef ZIP7_INC_LANG_PAGE_H
#define ZIP7_INC_LANG_PAGE_H

#include "../../../Windows/Control/PropertyPage.h"
#include "../../../Windows/Control/ComboBox.h"

struct CLangInfo
{
  unsigned NumLines;
  UString Name;
  UStringVector Comments;
  UStringVector MissingLines;
  UStringVector ExtraLines;
};

class CLangPage: public NWindows::NControl::CPropertyPage
{
  NWindows::NControl::CComboBox _langCombo;
  CObjectVector<CLangInfo> _langs;
  unsigned NumLangLines_EN;
  bool _needSave;
  
  void ShowLangInfo();
public:
  bool LangWasChanged;
  
  CLangPage(): _needSave(false), LangWasChanged(false) {}
  virtual bool OnInit() Z7_override;
  virtual void OnNotifyHelp() Z7_override;
  virtual bool OnCommand(unsigned code, unsigned itemID, LPARAM param) Z7_override;
  virtual LONG OnApply() Z7_override;
};

#endif
