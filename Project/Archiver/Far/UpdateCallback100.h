// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "../Common/IArchiveHandler2.h"

#include "Far/ProgressBox.h"

#include "Common/String.h"

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
  CProgressBox *m_ProgressBox;
public:
  void Init(IArchiveHandler100 *anArchiveHandler,
      CProgressBox *aProgressBox)
  {
    m_ArchiveHandler = anArchiveHandler;
    m_ProgressBox = aProgressBox;
  }
};



#endif
