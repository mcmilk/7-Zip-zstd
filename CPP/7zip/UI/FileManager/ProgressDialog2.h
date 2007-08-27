// ProgressDialog2.h

#ifndef __PROGRESSDIALOG2_H
#define __PROGRESSDIALOG2_H

#include "ProgressDialog2Res.h"

#include "Common/Types.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ProgressBar.h"
#include "Windows/Synchronization.h"

class CProgressSynch
{
  NWindows::NSynchronization::CCriticalSection _criticalSection;
  bool _stopped;
  bool _paused;
  UInt64 _total;
  UInt64 _completed;
  UInt64 _inSize;
  UInt64 _outSize;
  UInt64 _totalFiles;
  UInt64 _completedFiles;
  UString TitleFileName;
  UString CurrentFileName;
public:
  CProgressSynch(): 
      _stopped(false), _paused(false), 
      _total((UInt64)(Int64)-1), 
      _completed(0),
      _totalFiles((UInt64)(Int64)-1), 
      _completedFiles(0),
      _inSize((UInt64)(Int64)-1),
      _outSize((UInt64)(Int64)-1)
      {}

  bool GetStopped()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    return _stopped;
  }
  void SetStopped(bool value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _stopped = value;
  }
  bool GetPaused()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    return _paused;
  }
  void SetPaused(bool value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _paused = value;
  }
  void SetProgress(UInt64 total, UInt64 completed)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _total = total;
    _completed = completed;
  }
  void SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    if (inSize)
      _inSize = *inSize;
    if (outSize)
      _outSize = *outSize;
  }
  void SetPos(UInt64 completed)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _completed = completed;
  }
  void SetNumFilesTotal(UInt64 value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _totalFiles = value;
  }
  void SetNumFilesCur(UInt64 value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    _completedFiles = value;
  }
  HRESULT SetPosAndCheckPaused(UInt64 completed);
  void GetProgress(UInt64 &total, UInt64 &completed, 
    UInt64 &totalFiles, UInt64 &completedFiles, 
    UInt64 &inSize, UInt64 &outSize)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    total = _total;
    completed = _completed;
    totalFiles = _totalFiles;
    completedFiles = _completedFiles;
    inSize = _inSize;
    outSize = _outSize;
  }
  void SetTitleFileName(const UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    TitleFileName = fileName;
  }
  void GetTitleFileName(UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    fileName = TitleFileName;
  }
  void SetCurrentFileName(const UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    CurrentFileName = fileName;
  }
  void GetCurrentFileName(UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_criticalSection);
    fileName = CurrentFileName;
  }
};

class CU64ToI32Converter
{
  UInt64 _numShiftBits;
public:
  void Init(UInt64 _range);
  int Count(UInt64 aValue);
};

// class CProgressDialog: public NWindows::NControl::CModelessDialog

enum ESpeedMode
{
  kSpeedBytes,
  kSpeedKBytes,
  kSpeedMBytes
};

class CProgressDialog: public NWindows::NControl::CModalDialog
{
  UString _prevFileName;
  UString _prevTitleName;
private:
  UString backgroundString;
  UString backgroundedString;
  UString foregroundString;
  UString pauseString;
  UString continueString;
  UString pausedString;



  UINT_PTR _timer;

  UString _title;
  CU64ToI32Converter _converter;
  UInt64 _previousPos;
  UInt64 _range;
  NWindows::NControl::CProgressBar m_ProgressBar;

  UInt32 _prevPercentValue;
  UInt32 _prevTime;
  UInt32 _elapsedTime;
  UInt32 _prevElapsedSec;
  UInt64 _prevRemainingSec;
  ESpeedMode _prevMode;
  UInt64 _prevSpeed;

  bool _foreground;

  bool OnTimer(WPARAM timerID, LPARAM callback);
  void SetRange(UInt64 range);
  void SetPos(UInt64 pos);
  virtual bool OnInit();
  virtual void OnCancel();
  NWindows::NSynchronization::CManualResetEvent _dialogCreatedEvent;
  #ifndef _SFX
  void AddToTitle(LPCWSTR string);
  #endif

  void SetPauseText();
  void SetPriorityText();
  void OnPauseButton();
  void OnPriorityButton();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);

  void SetTitleText();
  void ShowSize(int id, UInt64 value);

public:
  CProgressSynch ProgressSynch;
  bool CompressingMode;
  
  #ifndef _SFX
  HWND MainWindow;
  UString MainTitle;
  UString MainAddTitle;
  ~CProgressDialog();
  #endif

  CProgressDialog(): _timer(0), CompressingMode(true)
    #ifndef _SFX
    ,MainWindow(0)
    #endif
  {
    if (_dialogCreatedEvent.Create() != S_OK)
      throw 1334987;
  }

  void WaitCreating() { _dialogCreatedEvent.Lock(); }


  INT_PTR Create(const UString &title, HWND wndParent = 0)
  { 
    _title = title;
    return CModalDialog::Create(IDD_DIALOG_PROGRESS, wndParent); 
  }

  static const UINT kCloseMessage;

  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

  void MyClose() { PostMessage(kCloseMessage);  };
};

#endif
