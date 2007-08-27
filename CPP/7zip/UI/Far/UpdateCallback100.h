// UpdateCallback.h

#ifndef __UPDATECALLBACK100_H
#define __UPDATECALLBACK100_H

#include "Common/MyCom.h"

#include "../Agent/IFolderArchive.h"

#include "ProgressBox.h"

class CUpdateCallback100Imp: 
  public IFolderArchiveUpdateCallback,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP

  // IProfress

  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IUpdateCallBack
  STDMETHOD(CompressOperation)(const wchar_t *aName);
  STDMETHOD(DeleteOperation)(const wchar_t *aName);
  STDMETHOD(OperationResult)(INT32 aOperationResult);
  STDMETHOD(UpdateErrorMessage)(const wchar_t *message);
  STDMETHOD(SetNumFiles)(UInt64 numFiles);

private:
  CMyComPtr<IInFolderArchive> m_ArchiveHandler;
  CProgressBox *m_ProgressBox;
public:
  void Init(IInFolderArchive *anArchiveHandler,
      CProgressBox *aProgressBox)
  {
    m_ArchiveHandler = anArchiveHandler;
    m_ProgressBox = aProgressBox;
  }
};



#endif
