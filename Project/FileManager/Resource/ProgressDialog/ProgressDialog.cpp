// ProgressDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "ProgressDialog.h"

static const UINT_PTR kTimerID = 3;
static const UINT kTimerElapse = 50;

#ifdef LANG        
#include "../../LangUtils.h"
#endif

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDCANCEL, 0x02000714 }
};
#endif

bool CProgressDialog::OnInit() 
{
  _range = UINT64(-1);

  #ifdef LANG        
  // LangSetWindowText(HWND(*this), 0x02000C00);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif

  SetText(_title);
  // _processStopped = false;
  m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));
  SetTimer(kTimerID, kTimerElapse);
  _dialogCreatedEvent.Set();
	return CModalDialog::OnInit();
}

void CProgressDialog::OnCancel() 
{
  _progressSynch.SetStopped(true);
  // _processStopped = true;
	// CModelessDialog::OnCancel();
}

void CProgressDialog::SetRange(UINT64 range)
{
  _range = range;
  _peviousPos = _UI64_MAX;
  _converter.Init(range);
  m_ProgressBar.SetRange32(0 , _converter.Count(range)); // Test it for 100%
}

void CProgressDialog::SetPos(UINT64 pos)
{
  bool redraw = true;
  if (pos < _range && pos > _peviousPos)
  {
    UINT32 aPosDelta = pos - _peviousPos;
    if (aPosDelta < (_range >> 10))
      redraw = false;
  }
  if(redraw)
  {
    m_ProgressBar.SetPos(_converter.Count(pos));  // Test it for 100%
    _peviousPos = pos;
  }
}

bool CProgressDialog::OnTimer(WPARAM timerID, LPARAM callback)
{
  UINT64 total, completed;
  _progressSynch.GetProgress(total, completed);
  if (total != _range)
    SetRange(total);
  SetPos(completed);
  return true;
}


////////////////////
// CU64ToI32Converter

static const UINT64 kMaxIntValue = 0x7FFFFFFF;

void CU64ToI32Converter::Init(UINT64 range)
{
  _numShiftBits = 0;
  while(range > kMaxIntValue)
  {
    range >>= 1;
    _numShiftBits++;
  }
}

int CU64ToI32Converter::Count(UINT64 aValue)
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
      End(0);
      // return true;
    }
  }
  return CModalDialog::OnMessage(message, wParam, lParam);
}

