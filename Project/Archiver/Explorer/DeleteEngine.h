// DeleteEngine.h

#pragma once

#ifndef __DELETEENGINE_H
#define __DELETEENGINE_H

#include "../Format/Common/IArchiveHandler.h"
// #include "ProgressDialog.h"
#include "Common/String.h"

/*
class CDeleteCallBackImp: 
  public IUpdateCallBack,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CDeleteCallBackImp)
  COM_INTERFACE_ENTRY(IUpdateCallBack)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDeleteCallBackImp)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(GetUpdateItemInfo)(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInClient,
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName);

  STDMETHOD(CompressOperation)(INT32 anIndex, IInStream **anInStream);
  STDMETHOD(DeleteOperation)(LPITEMIDLIST anItemIDList);
  
  STDMETHOD(OperationResult)(INT32 aOperationResult);

public:
  CProgressDialog m_ProcessDialog;
  ~CDeleteCallBackImp();
  void Init();
};
*/



#endif
