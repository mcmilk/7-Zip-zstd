// ProgressDialog2.cpp

#include "StdAfx.h"
#include "resource.h"
#include "ProgressDialog2.h"
#include "Common/IntToString.h"

using namespace NWindows;

static const UINT_PTR kTimerID = 3;
static const UINT kTimerElapse = 100;

#ifdef LANG        
#include "LangUtils.h"
#endif

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
  { IDC_BUTTON_PROGRESS_PRIORITY, 0x02000C10 },
  { IDC_BUTTON_PAUSE, 0x02000C12 },
  { IDCANCEL, 0x02000711 },
};
#endif

HRESULT CProgressSynch::SetPosAndCheckPaused(UInt64 completed)
{
  for (;;)
  {
    if(GetStopped())
      return E_ABORT;
    if(!GetPaused())
      break;
    ::Sleep(100);
  }
  SetPos(completed);
  return S_OK;
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

static const int kTitleFileNameSizeLimit = 36;
static const int kCurrentFileNameSizeLimit = 68;

static void ReduceString(UString &s, int size)
{
  if (s.Length() > size)
    s = s.Left(size / 2) + UString(L" ... ") + s.Right(size / 2);
}
#endif

bool CProgressDialog::OnInit() 
{
  _range = (UInt64)(Int64)(-1);
  _prevPercentValue = UInt32(-1);
  _prevElapsedSec = UInt32(-1);
  _prevRemainingSec = UInt32(-1);
  _prevSpeed = UInt32(-1);
  _prevMode = kSpeedBytes;
  _prevTime = ::GetTickCount();
  _elapsedTime = 0;
  _foreground = true;

  #ifdef LANG        
  // LangSetWindowText(HWND(*this), 0x02000C00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif


  CWindow window(GetItem(IDC_BUTTON_PROGRESS_PRIORITY));
  window.GetText(backgroundString);
  backgroundedString = backgroundString;
  backgroundedString.Replace(L"&", L"");

  window = GetItem(IDC_BUTTON_PAUSE);
  window.GetText(pauseString);

  foregroundString = LangString(IDS_PROGRESS_FOREGROUND, 0x02000C11);
  continueString = LangString(IDS_PROGRESS_CONTINUE, 0x02000C13);
  pausedString = LangString(IDS_PROGRESS_PAUSED, 0x02000C20);

  m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));
  _timer = SetTimer(kTimerID, kTimerElapse);
  _dialogCreatedEvent.Set();
  SetText(_title);
  SetPauseText();
  SetPriorityText();
  return CModalDialog::OnInit();
}

void CProgressDialog::OnCancel() 
{
  ProgressSynch.SetStopped(true);
}

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
  m_ProgressBar.SetRange32(0 , _converter.Count(range)); // Test it for 100%

  wchar_t s[32];
  ConvertSizeToString(_range, s);
  SetItemText(IDC_PROGRESS_TOTAL_VALUE, s);
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
    m_ProgressBar.SetPos(_converter.Count(pos));  // Test it for 100%
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

bool CProgressDialog::OnTimer(WPARAM /* timerID */, LPARAM /* callback */)
{
  if (ProgressSynch.GetPaused())
    return true;
  UInt64 total, completed, totalFiles, completedFiles, inSize, outSize;
  ProgressSynch.GetProgress(total, completed, totalFiles, completedFiles, inSize, outSize);

  UInt32 curTime = ::GetTickCount();

  if (total != _range)
    SetRange(total);
  if (total == (UInt64)(Int64)-1)
  {
    SetPos(0);
    SetRange(completed);
  }
  else
    SetPos(completed);

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

  if (elapsedChanged)
  {
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
    // if (elapsedChanged)
    {
      UInt64 speedB = (completed * 1000) / _elapsedTime;
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
    ProgressSynch.GetTitleFileName(titleName);
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
  ProgressSynch.GetCurrentFileName(fileName);
  if (_prevFileName != fileName)
  {
    ReduceString(fileName, kCurrentFileNameSizeLimit);
    SetItemText(IDC_PROGRESS_FILE_NAME, fileName);
    _prevFileName == fileName;
  }

  return true;
}


////////////////////
// CU64ToI32Converter

static const UInt64 kMaxIntValue = 0x7FFFFFFF;

void CU64ToI32Converter::Init(UInt64 range)
{
  _numShiftBits = 0;
  while(range > kMaxIntValue)
  {
    range >>= 1;
    _numShiftBits++;
  }
}

int CU64ToI32Converter::Count(UInt64 aValue)
{
  return int(aValue >> _numShiftBits);
}

const UINT CProgressDialog::kCloseMessage = WM_USER + 1;

bool CProgressDialog::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message)
  {
    case kCloseMessage:
    {
      KillTimer(_timer);
      _timer = 0;
      End(0);
      return true;
    }
    case WM_SETTEXT:
    {
      if (_timer == 0)
        return true;
    }
  }
  return CModalDialog::OnMessage(message, wParam, lParam);
}

void CProgressDialog::SetTitleText()
{
  UString title;
  if (ProgressSynch.GetPaused())
  {
    title = pausedString;
    title += L" ";
  }
  if (_prevPercentValue != UInt32(-1))
  {
    wchar_t s[64];
    ConvertUInt64ToString(_prevPercentValue, s);
    title += s;
    title += L"%";
  }
  if (!_foreground)
  {
    title += L" ";
    title += backgroundedString;
  }
  title += L" ";
  UString totalTitle = title + _title;
  UString fileName;
  ProgressSynch.GetTitleFileName(fileName);
  if (!fileName.IsEmpty())
  {
    ReduceString(fileName, kTitleFileNameSizeLimit);
    totalTitle += L" ";
    totalTitle += fileName;
  }
  SetText(totalTitle);
  #ifndef _SFX
  AddToTitle(title + MainAddTitle);
  #endif
}

void CProgressDialog::SetPauseText()
{
  SetItemText(IDC_BUTTON_PAUSE, ProgressSynch.GetPaused() ? 
    continueString : pauseString);
  SetTitleText();
}

void CProgressDialog::OnPauseButton()
{
  bool paused = !ProgressSynch.GetPaused();
  ProgressSynch.SetPaused(paused);
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
  SetPriorityClass(GetCurrentProcess(), _foreground ?
    NORMAL_PRIORITY_CLASS: IDLE_PRIORITY_CLASS);
  SetPriorityText();
}

bool CProgressDialog::OnButtonClicked(int buttonID, HWND buttonHWND) 
{ 
  switch(buttonID)
  {
    case IDCANCEL:
    {
      bool paused = ProgressSynch.GetPaused();;
      // ProgressSynch.SetPaused(true);
      if (!paused)
        OnPauseButton();
      int res = ::MessageBoxW(HWND(*this), 
          LangString(IDS_PROGRESS_ASK_CANCEL, 0x02000C30), 
          _title, MB_YESNOCANCEL);
      // ProgressSynch.SetPaused(paused);
      if (!paused)
        OnPauseButton();
      if (res == IDCANCEL || res == IDNO)
        return true;
      break;
    }
    case IDC_BUTTON_PAUSE:
      OnPauseButton();
      return true;
    case IDC_BUTTON_PROGRESS_PRIORITY:
    {
      OnPriorityButton();
      return true;
    }
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}
