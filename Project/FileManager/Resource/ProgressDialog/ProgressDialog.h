// ProgressDialog.h

#pragma once

#ifndef __PROGRESSDIALOG_H
#define __PROGRESSDIALOG_H

#include "resource.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ProgressBar.h"
#include "Windows/Synchronization.h"

class CProgressSynch
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
  bool _stopped;
  UINT64 _total;
  UINT64 _completed;
public:
  CProgressSynch(): _stopped(false),  _total(1), _completed(0) {}

  bool GetStopped()
  {
    NWindows::NSynchronization::CSingleLock aLock(&_criticalSection, true);
    return _stopped;
  }
  void SetStopped(bool value)
  {
    NWindows::NSynchronization::CSingleLock aLock(&_criticalSection, true);
    _stopped = value;
  }
  void SetProgress(UINT64 total, UINT64 completed)
  {
    NWindows::NSynchronization::CSingleLock aLock(&_criticalSection, true);
    _total = total;
    _completed = completed;
  }
  void SetPos(UINT64 completed)
  {
    NWindows::NSynchronization::CSingleLock aLock(&_criticalSection, true);
    _completed = completed;
  }
  void GetProgress(UINT64 &total, UINT64 &completed)
  {
    NWindows::NSynchronization::CSingleLock aLock(&_criticalSection, true);
    total = _total;
    completed = _completed;
  }
};

class CU64ToI32Converter
{
  UINT64 _numShiftBits;
public:
  void Init(UINT64 _range);
  int Count(UINT64 aValue);
};

// class CProgressDialog: public NWindows::NControl::CModelessDialog

class CProgressDialog: public NWindows::NControl::CModalDialog
{
private:
  CSysString _title;
  CU64ToI32Converter _converter;
  // bool _processStopped;
  UINT64 _peviousPos;
  UINT64 _range;
	NWindows::NControl::CProgressBar m_ProgressBar;
  bool OnTimer(WPARAM timerID, LPARAM callback);
  void SetRange(UINT64 range);
  void SetPos(UINT64 pos);
	virtual bool OnInit();
	virtual void OnCancel();
public:
  CProgressSynch _progressSynch;
  NWindows::NSynchronization::CManualResetEvent _dialogCreatedEvent;

	// CProgressDialog(CWnd* pParent = NULL);   // standard constructor
  // bool Create(HWND aWndParent = 0)
  //   { return CModelessDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_PROGRESS), aWndParent); }
  INT_PTR Create(const CSysString &title, HWND aWndParent = 0)
  { 
    _title = title;
    return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_PROGRESS), aWndParent); 
  }
  // bool WasProcessStopped() const { return _processStopped;}

  static const UINT kCloseMessage;

  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

  void MyClose()
  {
    PostMessage(kCloseMessage);
  };
};

#endif
