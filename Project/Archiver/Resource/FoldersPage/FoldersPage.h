// FoldersPage.h
 
#pragma once

#ifndef __FOLDERSPAGE_H
#define __FOLDERSPAGE_H

#include "Windows/Control/PropertyPage.h"

#include "../../Common/ZipSettings.h"

class CFoldersPage : public NWindows::NControl::CPropertyPage
{
  NZipSettings::NWorkDir::CInfo m_WorkDirInfo;

  void MyEnableControls();
  void ModifiedEvent();
  NWindows::NControl::CDialogChildControl m_WorkPath;
  NWindows::NControl::CDialogChildControl m_ButtonSetWorkPath;
  // int m_RadioWorkMode;
  void OnFoldersWorkButtonPath();
  int GetWorkMode() const;
  void GetWorkDir(NZipSettings::NWorkDir::CInfo &aWorkDirInfo);
  // bool WasChanged();
public:
  virtual bool OnInit();
  virtual bool OnCommand(int aCode, int anItemID, LPARAM lParam);
  virtual void OnNotifyHelp();
  virtual LONG OnApply();
  virtual bool OnButtonClicked(int aButtonID, HWND aButtonHWND);
};

#endif
