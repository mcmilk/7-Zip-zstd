// 7z/FolderInStream.h

#pragma once

#ifndef __7Z_FOLDERINSTREAM_H
#define __7Z_FOLDERINSTREAM_H

#include "ItemInfo.h"
#include "Header.h"
#include "ItemInfoUtils.h"

#include "Interface/IInOutStreams.h"

#include "../Common/IArchiveHandler.h"
#include "../Common/InStreamWithCRC.h"

class CFolderInStream: 
  public ISequentialInStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CFolderInStream)
  COM_INTERFACE_ENTRY(ISequentialInStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFolderInStream)
DECLARE_NO_REGISTRY()

  CFolderInStream();

  STDMETHOD(Read)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(ReadPart)(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:
  CComObjectNoLock<CInStreamWithCRC> *m_InStreamWithHashSpec;
  CComPtr<ISequentialInStream> m_InStreamWithHash;
  CComPtr<IUpdateCallBack> m_UpdateCallBack;

  bool m_FileIsOpen;
  UINT64 m_FilePos;

  const UINT32 *m_FileIndexes;
  UINT32 m_NumFiles;
  UINT32 m_FileIndex;

  HRESULT OpenStream();
  HRESULT CloseStream();
  void AddDigest();
public:
  void Init(IUpdateCallBack *anUpdateCallBack, 
      const UINT32 *aFileIndexes, UINT32 aNumFiles);
  CRecordVector<UINT32> m_CRCs;
  CRecordVector<UINT64> m_Sizes;
  UINT64 GetFullSize() const
  {
    UINT64 aSize = 0;
    for (int i = 0; i < m_Sizes.Size(); i++)      
      aSize += m_Sizes[i];
    return aSize;
  }
};
  

#endif
