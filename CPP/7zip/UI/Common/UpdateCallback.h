// UpdateCallback.h

#ifndef __UPDATE_CALLBACK_H
#define __UPDATE_CALLBACK_H

#include "../../../Common/MyCom.h"

#include "../../IPassword.h"
#include "../../ICoder.h"

#include "../Common/UpdatePair.h"
#include "../Common/UpdateProduce.h"

#define INTERFACE_IUpdateCallbackUI(x) \
  virtual HRESULT SetTotal(UInt64 size) x; \
  virtual HRESULT SetCompleted(const UInt64 *completeValue) x; \
  virtual HRESULT SetRatioInfo(const UInt64 *inSize, const UInt64 *outSize) x; \
  virtual HRESULT CheckBreak() x; \
  virtual HRESULT Finilize() x; \
  virtual HRESULT SetNumFiles(UInt64 numFiles) x; \
  virtual HRESULT GetStream(const wchar_t *name, bool isAnti) x; \
  virtual HRESULT OpenFileError(const wchar_t *name, DWORD systemError) x; \
  virtual HRESULT SetOperationResult(Int32 operationResult) x; \
  virtual HRESULT CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password) x; \
  virtual HRESULT CryptoGetTextPassword(BSTR *password) x; \
  /* virtual HRESULT ShowDeleteFile(const wchar_t *name) x; */ \
  /* virtual HRESULT CloseProgress() { return S_OK; }; */

struct IUpdateCallbackUI
{
  INTERFACE_IUpdateCallbackUI(=0)
};

struct CKeyKeyValPair
{
  UInt64 Key1;
  UInt64 Key2;
  unsigned Value;

  int Compare(const CKeyKeyValPair &a) const
  {
    if (Key1 < a.Key1) return -1;
    if (Key1 > a.Key1) return 1;
    return MyCompare(Key2, a.Key2);
  }
};


class CArchiveUpdateCallback:
  public IArchiveUpdateCallback2,
  public IArchiveGetRawProps,
  public IArchiveGetRootProps,
  public ICryptoGetTextPassword2,
  public ICryptoGetTextPassword,
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  bool _saclEnabled;
  #endif
  CRecordVector<CKeyKeyValPair> _map;

  UInt32 _hardIndex_From;
  UInt32 _hardIndex_To;

public:
  MY_UNKNOWN_IMP6(
      IArchiveUpdateCallback2,
      IArchiveGetRawProps,
      IArchiveGetRootProps,
      ICryptoGetTextPassword2,
      ICryptoGetTextPassword,
      ICompressProgressInfo)

  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);

  INTERFACE_IArchiveUpdateCallback2(;)
  INTERFACE_IArchiveGetRawProps(;)
  INTERFACE_IArchiveGetRootProps(;)

  STDMETHOD(CryptoGetTextPassword2)(Int32 *passwordIsDefined, BSTR *password);
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  CRecordVector<UInt64> VolumesSizes;
  FString VolName;
  FString VolExt;

  IUpdateCallbackUI *Callback;

  bool ShareForWrite;
  bool StdInMode;
  
  const CDirItems *DirItems;
  const CDirItem *ParentDirItem;
  
  const CObjectVector<CArcItem> *ArcItems;
  const CRecordVector<CUpdatePair2> *UpdatePairs;
  const UStringVector *NewNames;
  CMyComPtr<IInArchive> Archive;
  CMyComPtr<IArchiveGetRawProps> GetRawProps;
  CMyComPtr<IArchiveGetRootProps> GetRootProps;

  bool KeepOriginalItemNames;
  bool StoreNtSecurity;
  bool StoreHardLinks;
  bool StoreSymLinks;

  Byte *ProcessedItemsStatuses;

  CArchiveUpdateCallback();

  bool IsDir(const CUpdatePair2 &up) const
  {
    if (up.DirIndex >= 0)
      return DirItems->Items[up.DirIndex].IsDir();
    else if (up.ArcIndex >= 0)
      return (*ArcItems)[up.ArcIndex].IsDir;
    return false;
  }
};

#endif
