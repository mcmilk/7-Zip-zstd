// ProgressDialog2.h

#ifndef __PROGRESS_DIALOG2_H
#define __PROGRESS_DIALOG2_H

#include "Windows/Synchronization.h"
#include "Windows/Thread.h"

#include "Windows/Control/Dialog.h"
#include "Windows/Control/ListView.h"
#include "Windows/Control/ProgressBar.h"

class CProgressSync
{
  bool _stopped;
  bool _paused;
  bool _bytesProgressMode;

  UInt64 _totalBytes;
  UInt64 _curBytes;
  UInt64 _totalFiles;
  UInt64 _curFiles;
  UInt64 _inSize;
  UInt64 _outSize;
  
  UString _titleFileName;
  UString _currentFileName;

public:
  UStringVector Messages;
  UString ErrorMessage;
  UString ErrorMessageTitle;
  
  UString OkMessage;
  UString OkMessageTitle;

  NWindows::NSynchronization::CCriticalSection _cs;

  CProgressSync():
      _stopped(false), _paused(false),
      _totalBytes((UInt64)(Int64)-1), _curBytes(0),
      _totalFiles((UInt64)(Int64)-1), _curFiles(0),
      _inSize((UInt64)(Int64)-1),
      _outSize((UInt64)(Int64)-1),
      _bytesProgressMode(true)
      {}

  bool GetStopped()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    return _stopped;
  }
  void SetStopped(bool value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _stopped = value;
  }
  bool GetPaused()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    return _paused;
  }
  void SetPaused(bool value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _paused = value;
  }
  void SetBytesProgressMode(bool bytesProgressMode)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _bytesProgressMode = bytesProgressMode;
  }
  void SetProgress(UInt64 total, UInt64 completed)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _totalBytes = total;
    _curBytes = completed;
  }
  void SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    if (inSize)
      _inSize = *inSize;
    if (outSize)
      _outSize = *outSize;
  }
  void SetPos(UInt64 completed)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _curBytes = completed;
  }
  void SetNumBytesTotal(UInt64 value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _totalBytes = value;
  }
  void SetNumFilesTotal(UInt64 value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _totalFiles = value;
  }
  void SetNumFilesCur(UInt64 value)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _curFiles = value;
  }
  HRESULT ProcessStopAndPause();
  HRESULT SetPosAndCheckPaused(UInt64 completed);
  void GetProgress(UInt64 &total, UInt64 &completed,
    UInt64 &totalFiles, UInt64 &curFiles,
    UInt64 &inSize, UInt64 &outSize,
    bool &bytesProgressMode)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    total = _totalBytes;
    completed = _curBytes;
    totalFiles = _totalFiles;
    curFiles = _curFiles;
    inSize = _inSize;
    outSize = _outSize;
    bytesProgressMode = _bytesProgressMode;
  }
  void SetTitleFileName(const UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _titleFileName = fileName;
  }
  void GetTitleFileName(UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    fileName = _titleFileName;
  }
  void SetCurrentFileName(const UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    _currentFileName = fileName;
  }
  void GetCurrentFileName(UString &fileName)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    fileName = _currentFileName;
  }

  void AddErrorMessage(LPCWSTR message)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    Messages.Add(message);
  }

  void SetErrorMessage(const UString &message)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    ErrorMessage = message;
  }

  void SetOkMessage(const UString &message)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    OkMessage = message;
  }

  void SetOkMessageTitle(const UString &title)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    OkMessageTitle = title;
  }

  void SetErrorMessageTitle(const UString &title)
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(_cs);
    ErrorMessageTitle = title;
  }

  bool ThereIsMessage() const
  {
    return !Messages.IsEmpty() || !ErrorMessage.IsEmpty() || !OkMessage.IsEmpty();
  }
};

class CU64ToI32Converter
{
  UInt64 _numShiftBits;
public:
  void Init(UInt64 range)
  {
    // Windows CE doesn't like big number here.
    for (_numShiftBits = 0; range > (1 << 15); _numShiftBits++)
      range >>= 1;
  }
  int Count(UInt64 value) { return int(value >> _numShiftBits); }
};

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

  int buttonSizeX;
  int buttonSizeY;

  UINT_PTR _timer;

  UString _title;
  CU64ToI32Converter _converter;
  UInt64 _previousPos;
  UInt64 _range;
  NWindows::NControl::CProgressBar m_ProgressBar;
  NWindows::NControl::CListView _messageList;

  UInt32 _prevPercentValue;
  UInt32 _prevTime;
  UInt32 _elapsedTime;
  UInt32 _prevElapsedSec;
  UInt64 _prevRemainingSec;
  ESpeedMode _prevMode;
  UInt64 _prevSpeed;

  bool _foreground;

  int _numReduceSymbols;

  bool _wasCreated;
  bool _needClose;

  UInt32 _numPostedMessages;
  UInt32 _numAutoSizeMessages;

  bool _errorsWereDisplayed;

  bool _waitCloseByCancelButton;
  bool _cancelWasPressed;
  
  bool _inCancelMessageBox;
  bool _externalCloseMessageWasReceived;

  void UpdateStatInfo(bool showAll);
  bool OnTimer(WPARAM timerID, LPARAM callback);
  void SetRange(UInt64 range);
  void SetPos(UInt64 pos);
  virtual bool OnInit();
  virtual bool OnSize(WPARAM wParam, int xSize, int ySize);
  virtual void OnCancel();
  virtual void OnOK();
  NWindows::NSynchronization::CManualResetEvent _createDialogEvent;
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

  void UpdateMessagesDialog();

  void AddMessageDirect(LPCWSTR message);
  void AddMessage(LPCWSTR message);

  bool OnExternalCloseMessage();
  void EnableErrorsControls(bool enable);

  void ShowAfterMessages(HWND wndParent);

  void CheckNeedClose();
public:
  CProgressSync Sync;
  bool CompressingMode;
  bool WaitMode;
  bool ShowCompressionInfo;
  bool MessagesDisplayed; // = true if user pressed OK on all messages or there are no messages.
  int IconID;

  #ifndef _SFX
  HWND MainWindow;
  UString MainTitle;
  UString MainAddTitle;
  ~CProgressDialog();
  #endif

  CProgressDialog();
  void WaitCreating()
  {
    _createDialogEvent.Set();
    _dialogCreatedEvent.Lock();
  }


  INT_PTR Create(const UString &title, NWindows::CThread &thread, HWND wndParent = 0);


  virtual bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

  void ProcessWasFinished();
};


class CProgressCloser
{
  CProgressDialog *_p;
public:
  CProgressCloser(CProgressDialog &p) : _p(&p) {}
  ~CProgressCloser() { _p->ProcessWasFinished(); }
};

class CProgressThreadVirt
{
protected:
  UString ErrorMessage;
  UString ErrorPath1;
  UString ErrorPath2;
  UString OkMessage;
  UString OkMessageTitle;

  // error if any of HRESULT, ErrorMessage, ErrorPath
  virtual HRESULT ProcessVirt() = 0;
  void Process();
public:
  HRESULT Result;
  bool ThreadFinishedOK; // if there is no fatal exception
  CProgressDialog ProgressDialog;

  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    CProgressThreadVirt *p = (CProgressThreadVirt *)param;
    try
    {
      p->Process();
      p->ThreadFinishedOK = true;
    }
    catch (...) { p->Result = E_FAIL; }
    return 0;
  }

  HRESULT Create(const UString &title, HWND parentWindow = 0);
  CProgressThreadVirt(): Result(E_FAIL), ThreadFinishedOK(false) {}
};

UString HResultToMessage(HRESULT errorCode);

#endif
