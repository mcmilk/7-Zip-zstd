// UpdateCallback.h

#include "StdAfx.h"

#include "UpdateCallback100.h"
#include "ProcessMessages.h"

#include "Common/Defs.h"

STDMETHODIMP CUpdateCallBack100Imp::SetTotal(UINT64 aSize)
{
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  // m_Total = aSize;
  // m_ProgressDialog.Timer(PDTIMER_RESET);

  m_ProgressDialog.SetRange(aSize);
  m_ProgressDialog.SetPos(0);
  return S_OK;
}

STDMETHODIMP CUpdateCallBack100Imp::SetCompleted(const UINT64 *aCompleteValue)
{
  if (m_ThreadID != GetCurrentThreadId())
    return S_OK;
  ProcessMessages(m_ProgressDialog);
  if(m_ProgressDialog.WasProcessStopped())
  // if(m_ProgressDialog.HasUserCancelled())
    return E_ABORT;
  if (aCompleteValue != NULL)
    m_ProgressDialog.SetPos(*aCompleteValue);
    // m_ProgressDialog.SetProgress(*aCompleteValue, m_Total);
  return S_OK;
}

STDMETHODIMP CUpdateCallBack100Imp::CompressOperation(const wchar_t *aName)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallBack100Imp::DeleteOperation(const wchar_t *aName)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallBack100Imp::OperationResult(INT32 aOperationResult)
{
  return S_OK;
}
