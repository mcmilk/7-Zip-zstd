// UpdateCallback.h

#include "StdAfx.h"

#include "UpdateCallback100.h"

#include "Common/Defs.h"
#include "Far/FarUtils.h"

using namespace NFar;

STDMETHODIMP CUpdateCallBack100Imp::SetTotal(UINT64 aSize)
{
  if (m_ProgressBox != 0)
  {
    m_ProgressBox->SetTotal(aSize);
    m_ProgressBox->PrintCompeteValue(0);
  }
  return S_OK;
}

STDMETHODIMP CUpdateCallBack100Imp::SetCompleted(const UINT64 *aCompleteValue)
{
  if(WasEscPressed())
    return E_ABORT;
  if (m_ProgressBox != 0 && aCompleteValue != NULL)
    m_ProgressBox->PrintCompeteValue(*aCompleteValue);
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
