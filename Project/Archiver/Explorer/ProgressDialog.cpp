// ProgressDialog.cpp

#include "StdAfx.h"
#include "resource.h"
#include "ProgressDialog.h"

bool CProgressDialog::OnInit() 
{
  m_ProcessStopped = false;
  m_ProgressBar.Attach(GetItem(IDC_PROGRESS1));
	return CModelessDialog::OnInit();
}

void CProgressDialog::OnCancel() 
{
  m_ProcessStopped = true;
	// CModelessDialog::OnCancel();
}

void CProgressDialog::SetRange(UINT64 aRange)
{
  m_Range = aRange;
  m_PeviousPos = _UI64_MAX;
  m_Converter.Init(aRange);
  m_ProgressBar.SetRange32(0 , m_Converter.Count(aRange)); // Test it for 100%
}

void CProgressDialog::SetPos(UINT64 aPos)
{
  bool aRedraw = true;
  if (aPos < m_Range && aPos > m_PeviousPos)
  {
    UINT32 aPosDelta = aPos - m_PeviousPos;
    if (aPosDelta < (m_Range >> 10))
      aRedraw = false;
  }
  if(aRedraw)
  {
    m_ProgressBar.SetPos(m_Converter.Count(aPos));  // Test it for 100%
    m_PeviousPos = aPos;
  }
}

////////////////////
// CU64ToI32Converter

static const UINT64 kMaxIntValue = 0x7FFFFFFF;

void CU64ToI32Converter::Init(UINT64 aRange)
{
  m_NumShiftBits = 0;
  while(aRange > kMaxIntValue)
  {
    aRange >>= 1;
    m_NumShiftBits++;
  }
}

int CU64ToI32Converter::Count(UINT64 aValue)
{
  return int(aValue >> m_NumShiftBits);
}
