// ArchiveUpdateCallback.h

#pragma once

#ifndef __ARCHIVEUPDATECALLBACK_H
#define __ARCHIVEUPDATECALLBACK_H

#include "../../Archive/IArchive.h"
#include "../../IPassword.h"
#include "IFolderArchive.h"

#include "Common/String.h"
#include "Common/MyCom.h"

#include "../Common/UpdateProduce.h"
// #include "Interface/CryptoInterface.h"
// #include "Interface/MyCom.h"

class CArchiveUpdateCallback: 
  public IArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword2)


  // IProgress

  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IArchiveUpdateCallback
  // STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetUpdateItemInfo)(UINT32 index, 
      INT32 *newData, // 1 - new data, 0 - old data
      INT32 *newProperties, // 1 - new properties, 0 - old properties
      UINT32 *indexInArchive// set if existInArchive == true
      );

  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetStream)(UINT32 index, IInStream **anInStream);
  STDMETHOD(SetOperationResult)(INT32 operationResult);
  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);

private:
  CSysString m_BaseFolderPrefix;
  const CObjectVector<CDirItem> *m_DirItems;
  const CObjectVector<CArchiveItem> *m_ArchiveItems;
  const CObjectVector<CUpdatePair2> *m_UpdatePairs;
  CMyComPtr<IFolderArchiveUpdateCallback> m_UpdateCallback;
  CMyComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
  UINT m_CodePage;

  CMyComPtr<IInArchive> _inArchive;

public:
  void Init(const CSysString &baseFolderPrefix,
      const CObjectVector<CDirItem> *dirItems, 
      const CObjectVector<CArchiveItem> *archiveItems, // test CItemInfoExList
      CObjectVector<CUpdatePair2> *updatePairs,
      // UINT codePage,
      IInArchive *inArchive,
      IFolderArchiveUpdateCallback *updateCallback);
};




#endif
