// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "../Common/IArchiveHandler2.h"
#include "../Resource/ProgressDialog/ProgressDialog.h"
#include "resource.h"

#include "Common/String.h"

#ifdef LANG        
#include "../Common/LangUtils.h"
#endif

class CUpdateCallBack100Imp: 
  public IUpdateCallback100,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CUpdateCallBack100Imp)
  COM_INTERFACE_ENTRY(IUpdateCallback100)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CUpdateCallBack100Imp)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *aName);
  STDMETHOD(DeleteOperation)(const wchar_t *aName);
  STDMETHOD(OperationResult)(INT32 aOperationResult);

private:
  CComPtr<IArchiveHandler100> m_ArchiveHandler;
  CProgressDialog m_ProgressDialog;
  DWORD m_ThreadID;
  // UINT64 m_Total;
  // NWindows::NShell::CProgressDialog m_ProgressDialog;

public:
  ~CUpdateCallBack100Imp()
  {
    m_ProgressDialog.Destroy();
  }
  void Init(IArchiveHandler100 *anArchiveHandler, HWND aParentWindow)
  {
    m_ThreadID = GetCurrentThreadId();
    m_ProgressDialog.Create(aParentWindow);
    m_ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_COMPRESSING, 0x02000DC0));
    m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);
    m_ArchiveHandler = anArchiveHandler;
  }
};



#endif
