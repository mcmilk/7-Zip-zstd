// ProgressDialog.h

#pragma once

#ifndef __PROGRESSDIALOG_H
#define __PROGRESSDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ProgressBar.h"

class CU64ToI32Converter
{
  UINT64 m_NumShiftBits;
public:
  void Init(UINT64 m_Range);
  int Count(UINT64 aValue);
};

class CProgressDialog: public NWindows::NControl::CModelessDialog
{
private:
  CU64ToI32Converter m_Converter;
  bool m_ProcessStopped;
  UINT64 m_PeviousPos;
  UINT64 m_Range;
	NWindows::NControl::CProgressBar m_ProgressBar;
public:
	// CProgressDialog(CWnd* pParent = NULL);   // standard constructor
  bool Create(HWND aWndParent = 0)
    { return CModelessDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_PROGRESS), aWndParent); }
  void SetRange(UINT64 aRange);
  void SetPos(UINT64 aPos);
  bool WasProcessStopped() const { return m_ProcessStopped;}


protected:

	virtual bool OnInit();
	virtual void OnCancel();
};

#endif
