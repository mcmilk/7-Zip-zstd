// UpdateCallback.h

#ifndef __UPDATECALLBACK_H
#define __UPDATECALLBACK_H

#include "Common/MyCom.h"
#include "Common/MyString.h"

#include "../../IPassword.h"

#include "../Common/UpdatePair.h"
#include "../Common/UpdateProduce.h"

struct IUpdateCallbackUI
{
  virtual HRESULT SetTotal(UInt64 size) = 0;
  virtual HRESULT SetCompleted(const UInt64 *completeValue) = 0;
  virtual HRESULT CheckBreak() = 0;
  virtual HRESULT Finilize() = 0;
  virtual HRESULT GetStream(const wchar_t *name, bool isAnti) = 0;
  virtual HRESULT OpenFileError(const wchar_t *name, DWORD systemError) = 0;
  virtual HRESULT SetOperationResult(Int32 operationResult) = 0;
  virtual HRESULT CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password) = 0;
  virtual HRESULT CloseProgress() { return S_OK; };
};

class CArchiveUpdateCallback: 
  public IArchiveUpdateCallback2,
  public ICryptoGetTextPassword2,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IArchiveUpdateCallback2, 
      ICryptoGetTextPassword2)

  // IProgress
  STDMETHOD(SetTotal)(UInt64 size);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IUpdateCallback
  STDMETHOD(EnumProperties)(IEnumSTATPROPSTG **enumerator);  
  STDMETHOD(GetUpdateItemInfo)(UInt32 index, 
      Int32 *newData, Int32 *newProperties, UInt32 *indexInArchive);
  STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(UInt32 index, ISequentialInStream **inStream);
  STDMETHOD(SetOperationResult)(Int32 operationResult);

  STDMETHOD(GetVolumeSize)(UInt32 index, UInt64 *size);
  STDMETHOD(GetVolumeStream)(UInt32 index, ISequentialOutStream **volumeStream);

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);

public:
  CRecordVector<UInt64> VolumesSizes;
  UString VolName;
  UString VolExt;

  IUpdateCallbackUI *Callback;

  UString DirPrefix;
  bool ShareForWrite;
  bool StdInMode;
  const CObjectVector<CDirItem> *DirItems;
  const CObjectVector<CArchiveItem> *ArchiveItems;
  const CObjectVector<CUpdatePair2> *UpdatePairs;
  CMyComPtr<IInArchive> Archive;

  CArchiveUpdateCallback();
};

#endif
