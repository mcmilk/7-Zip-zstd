// UpdateCallback.h

#include "StdAfx.h"

#include "UpdateCallback100.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "FarUtils.h"

using namespace NFar;

STDMETHODIMP CUpdateCallback100Imp::SetNumFiles(UInt64 /* numFiles */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetTotal(UINT64 aSize)
{
  if (m_ProgressBox != 0)
  {
    m_ProgressBox->SetTotal(aSize);
    m_ProgressBox->PrintCompeteValue(0);
  }
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::SetCompleted(const UINT64 *aCompleteValue)
{
  if(WasEscPressed())
    return E_ABORT;
  if (m_ProgressBox != 0 && aCompleteValue != NULL)
    m_ProgressBox->PrintCompeteValue(*aCompleteValue);
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::CompressOperation(const wchar_t* /* name */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::DeleteOperation(const wchar_t* /* name */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::OperationResult(INT32 /* operationResult */)
{
  return S_OK;
}

STDMETHODIMP CUpdateCallback100Imp::UpdateErrorMessage(const wchar_t *message)
{
  CSysString s = UnicodeStringToMultiByte(message, CP_OEMCP);
  if (g_StartupInfo.ShowMessage(s) == -1)
    return E_ABORT;
  return S_OK;
}

