// SystemPage.h
 
#pragma once

#ifndef __SYSTEMPAGE_H
#define __SYSTEMPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ListView.h"

#include "../../Common/ArchiverInfo.h"

class CSystemPage: public NWindows::NControl::CPropertyPage
{
  bool _initMode;
  CObjectVector<CArchiverInfo> m_Archivers;
  NWindows::NControl::CListView m_ListView;
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnNotify(UINT aControlID, LPNMHDR lParam);
  virtual bool OnItemChanged(const NMLISTVIEW *info);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
};

#endif
