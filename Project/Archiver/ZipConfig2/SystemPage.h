// SystemPage.h
 
#pragma once

#ifndef __SYSTEMPAGE_H
#define __SYSTEMPAGE_H

#include "Windows/Control/PropertyPage.h"
#include "Windows/Control/ListView.h"

#include "../Common/ZipRegistryMain.h"

class CSystemPage: public NWindows::NControl::CPropertyPage
{
  CObjectVector<NZipRootRegistry::CArchiverInfo> m_Archivers;
  NWindows::NControl::CListView m_ListView;
public:
  virtual bool OnInit();
  virtual void OnNotifyHelp();
  virtual bool OnNotify(UINT aControlID, LPNMHDR lParam);
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
};

#endif
