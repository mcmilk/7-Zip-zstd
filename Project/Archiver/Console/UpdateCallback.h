// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK_H
#define __UPDATECALLBACK_H

#include "../Format/Common/ArchiveInterface.h"
#include "Interface/CryptoInterface.h"

#include "Common/String.h"

#include "../Common/UpdatePairInfo.h"
#include "../Common/UpdateProducer.h"

#include "PercentPrinter.h"

class CUpdateCallbackImp: 
  public IArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CUpdateCallbackImp)
  COM_INTERFACE_ENTRY(IArchiveUpdateCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CUpdateCallbackImp)

DECLARE_NO_REGISTRY()

  // IProfress

  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IUpdateCallback
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetUpdateItemInfo)(UINT32 index, 
      INT32 *newData, INT32 *newProperties, UINT32 *indexInArchive);

  STDMETHOD(GetProperty)(UINT32 index, PROPID propID, PROPVARIANT *value);

  STDMETHOD(GetStream)(UINT32 index, IInStream **inStream);
  
  STDMETHOD(SetOperationResult)(INT32 operationResult);

  STDMETHOD(CryptoGetTextPassword2)(INT32 *passwordIsDefined, BSTR *password);

private:
  const CArchiveStyleDirItemInfoVector *m_DirItems;
  const CArchiveItemInfoVector *m_ArchiveItems;
  const CUpdatePairInfo2Vector *m_UpdatePairs;

  CPercentPrinter m_PercentPrinter;

  bool m_EnablePercents;
  bool m_PercentCanBePrint;
  bool m_NeedBeClosed;

  bool _passwordIsDefined;
  UString _password;
  bool _askPassword;

public:
  CUpdateCallbackImp();
  ~CUpdateCallbackImp()
    { Finilize(); }
  void Init(const CArchiveStyleDirItemInfoVector *dirItems, 
      const CArchiveItemInfoVector *anArchiveItems, // test CItemInfoExList
      CUpdatePairInfo2Vector *anUpdatePairs, bool anEnablePercents,
      bool passwordIsDefined, const UString &password, bool askPassword);
  void Finilize();
};




#endif
