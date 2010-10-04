// ProgressDialog2.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/Control/Static.h"
#include "Windows/Error.h"

#include "ProgressDialog2.h"
#include "DialogSize.h"

#include "ProgressDialog2Res.h"

#include "../GUI/ExtractRes.h"

using namespace NWindows;

extern HINSTANCE g_hInstance;

static const UINT_PTR kTimerID = 3;

static const UINT kCloseMessage = WM_USER + 1;

static const UINT kTimerElapse =
  #ifdef UNDER_CE
  500
  #else
  100
  #endif
  ;

#include "LangUtils.h"

#ifdef LANG
static CIDLangPair kIDLangPairs[] =
{
  { IDCANCEL, 0x02000C00 },
  { IDC_PROGRESS_ELAPSED, 0x02000C01 },
  { IDC_PROGRESS_REMAINING, 0x02000C02 },
  { IDC_PROGRESS_TOTAL, 0x02000C03 },
  { IDC_PROGRESS_SPEED, 0x02000C04 },
  { IDC_PROGRESS_UNPACKED, 0x02000C05 },
  { IDC_PROGRESS_PACKED, 0x02000323 },
  { IDC_PROGRESS_RATIO, 0x02000C06 },
  { IDC_PROGRESS_SPEED, 0x02000C04 },
  { IDC_PROGRESS_FILES, 0x02000320 },
  { IDC_PROGRESS_ERRORS, 0x0308000A },
  { IDC_BUTTON_PROGRESS_PRIORITY, 0x02000C10 },
  { IDC_BUTTON_PAUSE, 0x02000C12 },
  { IDCANCEL, 0x02000711 },
};
#endif

HRESULT CProgressSync::ProcessStopAndPause()
{
  for (;;)
  {
    if (GetStopped())
      return E_ABORT;
    if (!GetPaused())
      break;
    ::Sleep(100);
  }
  return S_OK;
}

HRESULT CProgressSync::SetPosAndCheckPaused(UInt64 completed)
{
  RINOK(ProcessStopAndPause());
  SetPos(completed);
  return S_OK;
}


CProgressDialog::CProgressDialog(): _timer(0), CompressingMode(true)
    #ifndef _SFX
    , MainWindow(0)
    #endif
  {
    IconID = -1;
    MessagesDisplayed = false;
    _wasCreated = false;
    _needClose = false;
    _inCancelMessageBox = false;
    _externalCloseMessageWasReceived = false;

    _numPostedMessages = 0;
    _numAutoSizeMessages = 0;
    _errorsWereDisplayed = false;
    _waitCloseByCancelButton = false;
    _cancelWasPressed = false;
    ShowCompressionInfo = true;
    WaitMode = false;
    if (_dialogCreatedEvent.Create() != S_OK)
      throw 1334987;
    if (_createDialogEvent.Create() != S_OK)
      throw 1334987;
  }

#ifndef _SFX
CProgressDialog::~CProgressDialog()
{
  AddToTitle(L"");
}
void CProgressDialog::AddToTitle(LPCWSTR s)
{
  if (MainWindow != 0)
  {
    CWindow window(MainWindow);
    window.SetText(s + UString(MainTitle));
  }
}

#endif

static const int kTitleFileNameSizeLimit = 36;
static const int kCurrentFileNameSizeLimit = 82;

static void ReduceString(UString &s, int size)
{
  if (s.Length() > size)
    s = s.Left(size / 2) + UString(L" ... ") + s.Right(size / 2);
}

void CProgressDialog::EnableErrorsControls(bool enable)
{
  int cmdShow = enable ? SW_SHOW : SW_HIDE;
  ShowItem(IDC_PROGRESS_ERRORS, cmdShow);
  ShowItem(IDC_PROGRESS_ERRORS_VALUE, cmdShow);
  ShowItem(IDC_PROGRESS_LIST, cmdShow);
}

bool CProgressDialog::OnInit()
{
  _range = (UInt64)(Int64)-1;
  _prevPercentValue = (UInt32)-1;
  _prevElapsedSec = (UInt32)-1;
  _prevRemainingSec = (UInt32)-1;
  _prevSpeed = (UInt32)-1;
  _prevMode = kSpeedBytes;
  _prevTime = ::GetTickCount();
  _elapsedTime = 0;
  _foreground = true;

  m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));
  _messageList.Attach(GetItem(IDC_PROGRESS_LIST));

  _wasCreated = true;
  _dialogCreatedEvent.Set();

  #ifdef LANG
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif


  CWindow window(GetItem(IDC_BUTTON_PROGRESS_PRIORITY));
  window.GetText(backgroundString);
  backgroundedString = backgroundString;
  backgroundedString.Replace(L"&", L"");

  window = GetItem(IDC_BUTTON_PAUSE);
  window.GetText(pauseString);

  foregroundString = LangStringSpec(IDS_PROGRESS_FOREGROUND, 0x02000C11);
  continueString = LangStringSpec(IDS_PROGRESS_CONTINUE, 0x02000C13);
  pausedString = LangStringSpec(IDS_PROGRESS_PAUSED, 0x02000C20);

  SetText(_title);
  SetPauseText();
  SetPriorityText();


  #ifndef UNDER_CE
  _messageList.SetUnicodeFormat(true);
  #endif

  _messageList.InsertColumn(0, L"", 30);

  const UString s = LangStringSpec(IDS_MESSAGES_DIALOG_MESSAGE_COLUMN, 0x02000A80);

  _messageList.InsertColumn(1, s, 600);

  _messageList.SetColumnWidthAuto(0);
  _messageList.SetColumnWidthAuto(1);


  EnableErrorsControls(false);

  GetItemSizes(IDCANCEL, buttonSizeX, buttonSizeY);
  _numReduceSymbols = kCurrentFileNameSizeLimit;
  NormalizeSize(true);

  if (!ShowCompressionInfo)
  {
    HideItem(IDC_PROGRESS_PACKED);
    HideItem(IDC_PROGRESS_PACKED_VALUE);
    HideItem(IDC_PROGRESS_RATIO);
    HideItem(IDC_PROGRESS_RATIO_VALUE);
  }

  if (IconID >= 0)
  {
    HICON icon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IconID));
    // SetIcon(ICON_SMALL, icon);
    SetIcon(ICON_BIG, icon);
  }
  _timer = SetTimer(kTimerID, kTimerElapse);
  #ifdef UNDER_CE
  Foreground();
  #endif

  CheckNeedClose();

  return CModalDialog::OnInit();
}

bool CProgressDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  int sY;
  int sStep;
  int mx, my;
  {
    RECT rect;
    GetClientRectOfItem(IDC_PROGRESS_ELAPSED, rect);
    mx = rect.left;
    my = rect.top;
    sY = rect.bottom - rect.top;
    GetClientRectOfItem(IDC_PROGRESS_REMAINING, rect);
    sStep = rect.top - my;
  }


  InvalidateRect(NULL);

  int xSizeClient = xSize - mx * 2;

  {
    int i;
    for (i = 800; i > 40; i = i * 9 / 10)
      if (Units_To_Pixels_X(i) <= xSizeClient)
        break;
    _numReduceSymbols = i / 4;
  }

  int yPos = ySize - my - buttonSizeY;

  ChangeSubWindowSizeX(GetItem(IDC_PROGRESS_FILE_NAME), xSize - mx * 2);
  ChangeSubWindowSizeX(GetItem(IDC_PROGRESS1), xSize - mx * 2);

  int bSizeX = buttonSizeX;
  int mx2 = mx;
  for (;; mx2--)
  {
    int bSize2 = bSizeX * 3 + mx2 * 2;
    if (bSize2 <= xSizeClient)
      break;
    if (mx2 < 5)
    {
      bSizeX = (xSizeClient - mx2 * 2) / 3;
      break;
    }
  }
  if (bSizeX < 2)
    bSizeX = 2;

  {
    RECT rect;
    GetClientRectOfItem(IDC_PROGRESS_LIST, rect);
    int y = rect.top;
    int ySize2 = yPos - my - y;
    const int kMinYSize = buttonSizeY + buttonSizeY * 3 / 4;
    int xx = xSize - mx * 2;
    if (ySize2 < kMinYSize)
    {
      ySize2 = kMinYSize;
      if (xx > bSizeX * 2)
        xx -= bSizeX;
    }

    _messageList.Move(mx, y, xx, ySize2);
  }

  {
    int xPos = xSize - mx;
    xPos -= bSizeX;
    MoveItem(IDCANCEL, xPos, yPos, bSizeX, buttonSizeY);
    xPos -= (mx2 + bSizeX);
    MoveItem(IDC_BUTTON_PAUSE, xPos, yPos, bSizeX, buttonSizeY);
    xPos -= (mx2 + bSizeX);
    MoveItem(IDC_BUTTON_PROGRESS_PRIORITY, xPos, yPos, bSizeX, buttonSizeY);
  }

  int valueSize;
  int labelSize;
  int padSize;

  labelSize = Units_To_Pixels_X(MY_PROGRESS_LABEL_UNITS_MIN);
  valueSize = Units_To_Pixels_X(MY_PROGRESS_VALUE_UNITS);
  padSize = Units_To_Pixels_X(MY_PROGRESS_PAD_UNITS);
  int requiredSize = (labelSize + valueSize) * 2 + padSize;

  int gSize;
  {
    if (requiredSize < xSizeClient)
    {
      int incr = (xSizeClient - requiredSize) / 3;
      labelSize += incr;
    }
    else
      labelSize = (xSizeClient - valueSize * 2 - padSize) / 2;
    if (labelSize < 0)
      labelSize = 0;

    gSize = labelSize + valueSize;
    padSize = xSizeClient - gSize * 2;
  }

  labelSize = gSize - valueSize;

  UINT IDs[] =
  {
    IDC_PROGRESS_ELAPSED, IDC_PROGRESS_ELAPSED_VALUE,
    IDC_PROGRESS_REMAINING, IDC_PROGRESS_REMAINING_VALUE,
    IDC_PROGRESS_FILES, IDC_PROGRESS_FILES_VALUE,
    IDC_PROGRESS_RATIO, IDC_PROGRESS_RATIO_VALUE,
    IDC_PROGRESS_ERRORS, IDC_PROGRESS_ERRORS_VALUE,
      
    IDC_PROGRESS_TOTAL, IDC_PROGRESS_TOTAL_VALUE,
    IDC_PROGRESS_SPEED, IDC_PROGRESS_SPEED_VALUE,
    IDC_PROGRESS_UNPACKED, IDC_PROGRESS_UNPACKED_VALUE,
    IDC_PROGRESS_PACKED, IDC_PROGRESS_PACKED_VALUE
  };

  yPos = my;
  for (int i = 0; i < sizeof(IDs) / sizeof(IDs[0]); i += 2)
  {
    int x = mx;
    const int kNumColumn1Items = 5 * 2;
    if (i >= kNumColumn1Items)
    {
      if (i == kNumColumn1Items)
        yPos = my;
      x = mx + gSize + padSize;
    }
    MoveItem(IDs[i], x, yPos, labelSize, sY);
    MoveItem(IDs[i + 1], x + labelSize, yPos, valueSize, sY);
    yPos += sStep;
  }
  return false;
}

void CProgressDialog::OnCancel() { Sync.SetStopped(true); }
void CProgressDialog::OnOK() { }

static void ConvertSizeToString(UInt64 value, wchar_t *s)
{
  const wchar_t *kModif = L" KM";
  for (int i = 0; ; i++)
    if (i == 2 || value < (UInt64(10000) << (i * 10)))
    {
      ConvertUInt64ToString(value >> (i * 10), s);
      s += wcslen(s);
      *s++ = ' ';
      if (i != 0)
        *s++ = kModif[i];
      *s++ = L'B';
      *s++ = L'\0';
      return;
    }
}

void CProgressDialog::SetRange(UInt64 range)
{
  _range = range;
  _previousPos = (UInt64)(Int64)-1;
  _converter.Init(range);
  m_ProgressBar.SetRange32(0, _converter.Count(range));
}

void CProgressDialog::SetPos(UInt64 pos)
{
  bool redraw = true;
  if (pos < _range && pos > _previousPos)
  {
    if (pos - _previousPos < (_range >> 10))
      redraw = false;
  }
  if(redraw)
  {
    m_ProgressBar.SetPos(_converter.Count(pos)); // Test it for 100%
    _previousPos = pos;
  }
}

static void GetTimeString(UInt64 timeValue, TCHAR *s)
{
  wsprintf(s, TEXT("%02d:%02d:%02d"),
      UInt32(timeValue / 3600),
      UInt32((timeValue / 60) % 60),
      UInt32(timeValue % 60));
}

void CProgressDialog::ShowSize(int id, UInt64 value)
{
  wchar_t s[40];
  s[0] = 0;
  if (value != (UInt64)(Int64)-1)
    ConvertSizeToString(value, s);
  SetItemText(id, s);
}

void CProgressDialog::UpdateStatInfo(bool showAll)
{
  UInt64 total, completed, totalFiles, completedFiles, inSize, outSize;
  bool bytesProgressMode;
  Sync.GetProgress(total, completed, totalFiles, completedFiles, inSize, outSize, bytesProgressMode);

  UInt32 curTime = ::GetTickCount();

  UInt64 progressTotal = bytesProgressMode ? total : totalFiles;
  UInt64 progressCompleted = bytesProgressMode ? completed : completedFiles;

  if (progressTotal != _range)
    SetRange(progressTotal);
  if (progressTotal == (UInt64)(Int64)-1)
  {
    SetPos(0);
    SetRange(progressCompleted);
  }
  else
    SetPos(progressCompleted);

  wchar_t s[32] = { 0 };
  if (total != (UInt64)(Int64)-1)
    ConvertSizeToString(total, s);
  SetItemText(IDC_PROGRESS_TOTAL_VALUE, s);

  _elapsedTime += (curTime - _prevTime);
  _prevTime = curTime;

  UInt32 elapsedSec = _elapsedTime / 1000;

  bool elapsedChanged = false;
  if (elapsedSec != _prevElapsedSec)
  {
    TCHAR s[40];
    GetTimeString(elapsedSec, s);
    SetItemText(IDC_PROGRESS_ELAPSED_VALUE, s);
    _prevElapsedSec = elapsedSec;
    elapsedChanged = true;
  }

  if (elapsedChanged || showAll)
  {
    {
      UInt64 numErrors;

      {
        NWindows::NSynchronization::CCriticalSectionLock lock(Sync._cs);
        numErrors = Sync.Messages.Size();
      }
      if (numErrors > 0)
      {
        UpdateMessagesDialog();
        TCHAR s[40];
        ConvertUInt64ToString(numErrors, s);
        SetItemText(IDC_PROGRESS_ERRORS_VALUE, s);
        if (!_errorsWereDisplayed)
        {
          _errorsWereDisplayed = true;
          EnableErrorsControls(true);
        }
      }
    }

    if (completed != 0)
    {

    if (total == (UInt64)(Int64)-1)
    {
      SetItemText(IDC_PROGRESS_REMAINING_VALUE, L"");
    }
    else
    {
      UInt64 remainingTime = 0;
      if (completed < total)
        remainingTime = _elapsedTime * (total - completed)  / completed;
      UInt64 remainingSec = remainingTime / 1000;
      if (remainingSec != _prevRemainingSec)
      {
        TCHAR s[40];
        GetTimeString(remainingSec, s);
        SetItemText(IDC_PROGRESS_REMAINING_VALUE, s);
        _prevRemainingSec = remainingSec;
      }
    }
    {
      UInt32 elapsedTime = (_elapsedTime == 0) ? 1 : _elapsedTime;
      UInt64 speedB = (completed * 1000) / elapsedTime;
      UInt64 speedKB = speedB / 1024;
      UInt64 speedMB = speedKB / 1024;
      const UInt32 kLimit1 = 10;
      TCHAR s[40];
      bool needRedraw = false;
      if (speedMB >= kLimit1)
      {
        if (_prevMode != kSpeedMBytes || speedMB != _prevSpeed)
        {
          ConvertUInt64ToString(speedMB, s);
          lstrcat(s, TEXT(" MB/s"));
          _prevMode = kSpeedMBytes;
          _prevSpeed = speedMB;
          needRedraw = true;
        }
      }
      else if (speedKB >= kLimit1)
      {
        if (_prevMode != kSpeedKBytes || speedKB != _prevSpeed)
        {
          ConvertUInt64ToString(speedKB, s);
          lstrcat(s, TEXT(" KB/s"));
          _prevMode = kSpeedKBytes;
          _prevSpeed = speedKB;
          needRedraw = true;
        }
      }
      else
      {
        if (_prevMode != kSpeedBytes || speedB != _prevSpeed)
        {
          ConvertUInt64ToString(speedB, s);
          lstrcat(s, TEXT(" B/s"));
          _prevMode = kSpeedBytes;
          _prevSpeed = speedB;
          needRedraw = true;
        }
      }
      if (needRedraw)
        SetItemText(IDC_PROGRESS_SPEED_VALUE, s);
    }
    }

    if (total == 0)
      total = 1;
    UInt32 percentValue = (UInt32)(completed * 100 / total);
    UString titleName;
    Sync.GetTitleFileName(titleName);
    if (percentValue != _prevPercentValue || _prevTitleName != titleName)
    {
      _prevPercentValue = percentValue;
      SetTitleText();
      _prevTitleName = titleName;
    }
    
    TCHAR s[64];
    ConvertUInt64ToString(completedFiles, s);
    if (totalFiles != (UInt64)(Int64)-1)
    {
      lstrcat(s, TEXT(" / "));
      ConvertUInt64ToString(totalFiles, s + lstrlen(s));
    }
    
    SetItemText(IDC_PROGRESS_FILES_VALUE, s);
    
    const UInt64 packSize   = CompressingMode ? outSize : inSize;
    const UInt64 unpackSize = CompressingMode ? inSize : outSize;

    if (unpackSize == (UInt64)(Int64)-1 && packSize == (UInt64)(Int64)-1)
    {
      ShowSize(IDC_PROGRESS_UNPACKED_VALUE, completed);
      SetItemText(IDC_PROGRESS_PACKED_VALUE, L"");
    }
    else
    {
      ShowSize(IDC_PROGRESS_UNPACKED_VALUE, unpackSize);
      ShowSize(IDC_PROGRESS_PACKED_VALUE, packSize);
      
      if (packSize != (UInt64)(Int64)-1 && unpackSize != (UInt64)(Int64)-1 && unpackSize != 0)
      {
        UInt64 ratio = packSize * 100 / unpackSize;
        ConvertUInt64ToString(ratio, s);
        lstrcat(s, TEXT("%"));
        SetItemText(IDC_PROGRESS_RATIO_VALUE, s);
      }
    }
  }


  UString fileName;
  Sync.GetCurrentFileName(fileName);
  if (_prevFileName != fileName)
  {
    int slashPos = fileName.ReverseFind(WCHAR_PATH_SEPARATOR);
    UString s1, s2;
    if (slashPos >= 0)
    {
      s1 = fileName.Left(slashPos + 1);
      s2 = fileName.Mid(slashPos + 1);
    }
    else
      s2 = fileName;
    ReduceString(s1, _numReduceSymbols);
    ReduceString(s2, _numReduceSymbols);
    UString s = s1 + L"\n" + s2;
    SetItemText(IDC_PROGRESS_FILE_NAME, s);
    _prevFileName == fileName;
  }
}

bool CProgressDialog::OnTimer(WPARAM /* timerID */, LPARAM /* callback */)
{
  if (Sync.GetPaused())
    return true;

  CheckNeedClose();

  UpdateStatInfo(false);
  return true;
}

struct CWaitCursor
{
  HCURSOR _waitCursor;
  HCURSOR _oldCursor;
  CWaitCursor()
  {
    _waitCursor = LoadCursor(NULL, IDC_WAIT);
    if (_waitCursor != NULL)
      _oldCursor = SetCursor(_waitCursor);
  }
  ~CWaitCursor()
  {
    if (_waitCursor != NULL)
      SetCursor(_oldCursor);
  }
};

INT_PTR CProgressDialog::Create(const UString &title, NWindows::CThread &thread, HWND wndParent)
{
  INT_PTR res = 0;
  try
  {
    if (WaitMode)
    {
      CWaitCursor waitCursor;
      HANDLE h[] = { thread, _createDialogEvent };
      
      WRes res = WaitForMultipleObjects(sizeof(h) / sizeof(h[0]), h, FALSE,
          #ifdef UNDER_CE
          2500
          #else
          1000
          #endif
          );
      if (res == WAIT_OBJECT_0 && !Sync.ThereIsMessage())
        return 0;
    }
    _title = title;
    BIG_DIALOG_SIZE(360, 192);
    res = CModalDialog::Create(SIZED_DIALOG(IDD_DIALOG_PROGRESS), wndParent);
  }
  catch(...)
  {
    _wasCreated = true;
    _dialogCreatedEvent.Set();
    res = res;
  }
  thread.Wait();
  if (!MessagesDisplayed)
    MessageBoxW(wndParent, L"Progress Error", L"7-Zip", MB_ICONERROR | MB_OK);
  return res;
}

bool CProgressDialog::OnExternalCloseMessage()
{
  UpdateStatInfo(true);
  
  HideItem(IDC_BUTTON_PROGRESS_PRIORITY);
  HideItem(IDC_BUTTON_PAUSE);
  SetItemText(IDCANCEL, LangStringSpec(IDS_CLOSE, 0x02000713));
  
  bool thereAreMessages;
  UString okMessage;
  UString okMessageTitle;
  UString errorMessage;
  UString errorMessageTitle;
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(Sync._cs);
    errorMessage = Sync.ErrorMessage;
    errorMessageTitle = Sync.ErrorMessageTitle;
    okMessage = Sync.OkMessage;
    okMessageTitle = Sync.OkMessageTitle;
    thereAreMessages = !Sync.Messages.IsEmpty();
  }
  if (!errorMessage.IsEmpty())
  {
    MessagesDisplayed = true;
    if (errorMessageTitle.IsEmpty())
      errorMessageTitle = L"7-Zip";
    MessageBoxW(*this, errorMessage, errorMessageTitle, MB_ICONERROR | MB_OK);
  }
  else if (!thereAreMessages)
  {
    MessagesDisplayed = true;
    if (!okMessage.IsEmpty())
    {
      if (okMessageTitle.IsEmpty())
        okMessageTitle = L"7-Zip";
      MessageBoxW(*this, okMessage, okMessageTitle, MB_OK);
    }
  }

  if (thereAreMessages && !_cancelWasPressed)
  {
    _waitCloseByCancelButton = true;
    UpdateMessagesDialog();
    return true;
  }

  End(0);
  return true;
}

bool CProgressDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case kCloseMessage:
    {
      KillTimer(_timer);
      _timer = 0;
      if (_inCancelMessageBox)
      {
        _externalCloseMessageWasReceived = true;
        break;
      }
      return OnExternalCloseMessage();
    }
    /*
    case WM_SETTEXT:
    {
      if (_timer == 0)
        return true;
      break;
    }
    */
  }
  return CModalDialog::OnMessage(message, wParam, lParam);
}

void CProgressDialog::SetTitleText()
{
  UString title;
  if (Sync.GetPaused())
  {
    title = pausedString;
    title += L' ';
  }
  if (_prevPercentValue != (UInt32)-1)
  {
    wchar_t s[64];
    ConvertUInt64ToString(_prevPercentValue, s);
    title += s;
    title += L'%';
  }
  if (!_foreground)
  {
    title += L' ';
    title += backgroundedString;
  }
  title += L' ';
  UString totalTitle = title + _title;
  UString fileName;
  Sync.GetTitleFileName(fileName);
  if (!fileName.IsEmpty())
  {
    ReduceString(fileName, kTitleFileNameSizeLimit);
    totalTitle += L' ';
    totalTitle += fileName;
  }
  SetText(totalTitle);
  #ifndef _SFX
  AddToTitle(title + MainAddTitle);
  #endif
}

void CProgressDialog::SetPauseText()
{
  SetItemText(IDC_BUTTON_PAUSE, Sync.GetPaused() ?
    continueString : pauseString);
  SetTitleText();
}

void CProgressDialog::OnPauseButton()
{
  bool paused = !Sync.GetPaused();
  Sync.SetPaused(paused);
  UInt32 curTime = ::GetTickCount();
  if (paused)
    _elapsedTime += (curTime - _prevTime);
  _prevTime = curTime;
  SetPauseText();
}

void CProgressDialog::SetPriorityText()
{
  SetItemText(IDC_BUTTON_PROGRESS_PRIORITY, _foreground ?
      backgroundString :
      foregroundString);
  SetTitleText();
}

void CProgressDialog::OnPriorityButton()
{
  _foreground = !_foreground;
  #ifndef UNDER_CE
  SetPriorityClass(GetCurrentProcess(), _foreground ?
    NORMAL_PRIORITY_CLASS: IDLE_PRIORITY_CLASS);
  #endif
  SetPriorityText();
}

void CProgressDialog::AddMessageDirect(LPCWSTR message)
{
  int itemIndex = _messageList.GetItemCount();
  wchar_t sz[32];
  ConvertInt64ToString(itemIndex, sz);
  _messageList.InsertItem(itemIndex, sz);
  _messageList.SetSubItem(itemIndex, 1, message);
}

void CProgressDialog::AddMessage(LPCWSTR message)
{
  UString s = message;
  while (!s.IsEmpty())
  {
    int pos = s.Find(L'\n');
    if (pos < 0)
      break;
    AddMessageDirect(s.Left(pos));
    s.Delete(0, pos + 1);
  }
  AddMessageDirect(s);
}

static unsigned GetNumDigits(UInt32 value)
{
  unsigned i;
  for (i = 0; value >= 10; i++)
    value /= 10;
  return i;
}

void CProgressDialog::UpdateMessagesDialog()
{
  UStringVector messages;
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(Sync._cs);
    for (int i = _numPostedMessages; i < Sync.Messages.Size(); i++)
      messages.Add(Sync.Messages[i]);
    _numPostedMessages = Sync.Messages.Size();
  }
  if (!messages.IsEmpty())
  {
    for (int i = 0; i < messages.Size(); i++)
      AddMessage(messages[i]);
    if (_numAutoSizeMessages < 256 || GetNumDigits(_numPostedMessages) > GetNumDigits(_numAutoSizeMessages))
    {
      _messageList.SetColumnWidthAuto(0);
      _messageList.SetColumnWidthAuto(1);
      _numAutoSizeMessages = _numPostedMessages;
    }
  }
}


bool CProgressDialog::OnButtonClicked(int buttonID, HWND buttonHWND)
{
  switch(buttonID)
  {
    // case IDOK: // if IDCANCEL is not DEFPUSHBUTTON
    case IDCANCEL:
    {
      if (_waitCloseByCancelButton)
      {
        MessagesDisplayed = true;
        End(IDCLOSE);
        break;
      }
        
      bool paused = Sync.GetPaused();
      if (!paused)
        OnPauseButton();
      _inCancelMessageBox = true;
      int res = ::MessageBoxW(HWND(*this),
          LangStringSpec(IDS_PROGRESS_ASK_CANCEL, 0x02000C30),
          _title, MB_YESNOCANCEL);
      _inCancelMessageBox = false;
      if (!paused)
        OnPauseButton();
      if (res == IDCANCEL || res == IDNO)
      {
        if (_externalCloseMessageWasReceived)
          OnExternalCloseMessage();
        return true;
      }

      _cancelWasPressed = true;
      MessagesDisplayed = true;
      break;
    }

    case IDC_BUTTON_PAUSE:
      OnPauseButton();
      return true;
    case IDC_BUTTON_PROGRESS_PRIORITY:
      OnPriorityButton();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CProgressDialog::CheckNeedClose()
{
  if (_needClose)
  {
    PostMessage(kCloseMessage);
    _needClose = false;
  }
}

void CProgressDialog::ProcessWasFinished()
{
  // Set Window title here.
  if (!WaitMode)
    WaitCreating();
  
  if (_wasCreated)
    PostMessage(kCloseMessage);
  else
    _needClose = true;
}


HRESULT CProgressThreadVirt::Create(const UString &title, HWND parentWindow)
{
  NWindows::CThread thread;
  RINOK(thread.Create(MyThreadFunction, this));
  ProgressDialog.Create(title, thread, parentWindow);
  return S_OK;
}

UString HResultToMessage(HRESULT errorCode)
{
  UString message;
  if (errorCode == E_OUTOFMEMORY)
    message = LangStringSpec(IDS_MEM_ERROR, 0x0200060B);
  else if (!NError::MyFormatMessage(errorCode, message))
    message.Empty();
  if (message.IsEmpty())
    message = L"Error";
  return message;
}

static void AddMessageToString(UString &dest, const UString &src)
{
  if (!src.IsEmpty())
  {
    if (!dest.IsEmpty())
      dest += L'\n';
    dest += src;
  }
}

void CProgressThreadVirt::Process()
{
  CProgressCloser closer(ProgressDialog);
  UString m;
  try { Result = ProcessVirt(); }
  catch(const wchar_t *s) { m = s; }
  catch(const UString &s) { m = s; }
  catch(const char *s) { m = GetUnicodeString(s); }
  catch(...) { m = L"Error"; }
  if (Result != E_ABORT)
  {
    if (m.IsEmpty() && Result != S_OK)
      m = HResultToMessage(Result);
  }
  AddMessageToString(m, ErrorMessage);
  AddMessageToString(m, ErrorPath1);
  AddMessageToString(m, ErrorPath2);

  if (m.IsEmpty())
  {
    if (!OkMessage.IsEmpty())
    {
      ProgressDialog.Sync.SetOkMessageTitle(OkMessageTitle);
      ProgressDialog.Sync.SetOkMessage(OkMessage);
    }
  }
  else
  {
    ProgressDialog.Sync.SetErrorMessage(m);
    if (Result == S_OK)
      Result = E_FAIL;
  }
}
