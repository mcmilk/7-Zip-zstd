// DeleteEngine.h

#include "StdAfx.h"

/*
#include "DeleteEngine.h"
#include "MessagesDialog.h"
#include "ProcessMessages.h"
#include "Common/Defs.h"
*/

//using namespace NWindows;
static const kStepSize = 1 << 17;

////////////////////////////////
// CDeleteCallBackImp

/*
CDeleteCallBackImp::~CDeleteCallBackImp()
{
  m_ProcessDialog.DestroyWindow();
}

void CDeleteCallBackImp::Init()
{
}

STDMETHODIMP CDeleteCallBackImp::SetTotal(UINT64 aSize)
{
  m_ProcessDialog.SetRange(aSize);
  m_ProcessDialog.SetPos(0);
  return S_OK;
}

STDMETHODIMP CDeleteCallBackImp::SetCompleted(const UINT64 *aCompleteValue)
{
  ProcessMessages();
  if(m_ProcessDialog.WasProcessStopped())
    return E_ABORT;
  if (aCompleteValue != NULL)
    m_ProcessDialog.SetPos(*aCompleteValue);
  return S_OK;
}

STDMETHODIMP CDeleteCallBackImp::GetUpdateItemInfo(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInClient,
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName)
{
  return E_FAIL;
}

STDMETHODIMP CDeleteCallBackImp::CompressOperation(INT32 anIndex,
    IInStream **anInStream)
{
  return E_FAIL;
}

STDMETHODIMP CDeleteCallBackImp::DeleteOperation(LPITEMIDLIST anItemIDList)
{
  return S_OK;
}

STDMETHODIMP CDeleteCallBackImp::OperationResult(INT32 aOperationResult)
{
  return S_OK;
}
*/