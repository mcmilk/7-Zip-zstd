// UpdateCallback.h

#pragma once

#ifndef __UPDATECALLBACK_H
#define __UPDATECALLBACK_H

// #include "../Format/Common/ArchiveInterface.h"
#include "Common/MyCom.h"
#include "Common/String.h"

#include "../../IPassword.h"

#include "../Common/UpdatePair.h"
#include "../Common/UpdateProduce.h"

#include "PercentPrinter.h"

class CUpdateCallbackImp: 
  public IArchiveUpdateCallback,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword2)

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
  const CObjectVector<CDirItem> *m_DirItems;
  const CObjectVector<CArchiveItem> *m_ArchiveItems;
  const CObjectVector<CUpdatePair2> *m_UpdatePairs;

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
  void Init(
      const CObjectVector<CDirItem> *dirItems, 
      const CObjectVector<CArchiveItem> *archiveItems, // test CItemInfoExList
      CObjectVector<CUpdatePair2> *updatePairs, 
      bool enablePercents,
      bool passwordIsDefined, 
      const UString &password, 
      bool askPassword);
  void Finilize();
};




#endif
