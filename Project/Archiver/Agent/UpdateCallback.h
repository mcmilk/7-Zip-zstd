// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK_H
#define __UPDATECALLBACK_H

#include "../Format/Common/IArchiveHandler.h"
#include "../Common/IArchiveHandler2.h"

#include "Common/String.h"

#include "../Common/UpdatePairInfo.h"
#include "../Common/UpdateProducer.h"
#include "../Format/Common/FormatCryptoInterface.h"

class CUpdateCallBackImp: 
  public IUpdateCallBack,
  public ICryptoGetTextPassword2,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CUpdateCallBackImp)
  COM_INTERFACE_ENTRY(IUpdateCallBack)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CUpdateCallBackImp)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(GetUpdateItemInfo)(INT32 anIndex, 
      INT32 *anCompress, // 1 - compress 0 - copy
      INT32 *anExistInArchive, // 1 - exist, 0 - not exist
      INT32 *anIndexInServer,
      UINT32 *anAttributes,
      FILETIME *aCreationTime, 
      FILETIME *aLastAccessTime, 
      FILETIME *aLastWriteTime, 
      UINT64 *aSize, 
      BSTR *aName);

  STDMETHOD(CompressOperation)(INT32 anIndex, IInStream **anInStream);
  STDMETHOD(DeleteOperation)(LPITEMIDLIST anItemIDList);
  
  STDMETHOD(OperationResult)(INT32 aOperationResult);
  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);

private:
  CSysString m_BaseFolderPrefix;
  const CArchiveStyleDirItemInfoVector *m_DirItems;
  const CArchiveItemInfoVector *m_ArchiveItems;
  const CUpdatePairInfo2Vector *m_UpdatePairs;
  CComPtr<IUpdateCallback100> m_UpdateCallback;
  CComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
  UINT m_CodePage;

public:
  void Init(const CSysString &aBaseFolderPrefix,
      const CArchiveStyleDirItemInfoVector *DirItems, 
      const CArchiveItemInfoVector *anArchiveItems, // test CItemInfoExList
      CUpdatePairInfo2Vector *anUpdatePairs,
      UINT aCodePage,
      IUpdateCallback100 *anUpdateCallback);
};




#endif
