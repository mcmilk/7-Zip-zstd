// ArchiveUpdateCallback.h

#pragma once

#ifndef __ARCHIVEUPDATECALLBACK_H
#define __ARCHIVEUPDATECALLBACK_H

#include "../Format/Common/ArchiveInterface.h"
#include "../Common/FolderArchiveInterface.h"

#include "Common/String.h"

#include "../Common/UpdatePairInfo.h"
#include "../Common/UpdateProducer.h"
#include "Interface/CryptoInterface.h"

class CArchiveUpdateCallback: 
  public IArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CArchiveUpdateCallback)
  COM_INTERFACE_ENTRY(IArchiveUpdateCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CArchiveUpdateCallback)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IArchiveUpdateCallback
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
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
  const CArchiveStyleDirItemInfoVector *m_DirItems;
  const CArchiveItemInfoVector *m_ArchiveItems;
  const CUpdatePairInfo2Vector *m_UpdatePairs;
  CComPtr<IFolderArchiveUpdateCallback> m_UpdateCallback;
  CComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
  UINT m_CodePage;

  CComPtr<IInArchive> _inArchive;

public:
  void Init(const CSysString &baseFolderPrefix,
      const CArchiveStyleDirItemInfoVector *dirItems, 
      const CArchiveItemInfoVector *archiveItems, // test CItemInfoExList
      CUpdatePairInfo2Vector *updatePairs,
      // UINT codePage,
      IInArchive *inArchive,
      IFolderArchiveUpdateCallback *updateCallback);
};




#endif
