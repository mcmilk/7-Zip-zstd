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
#include "../../../Compress/Interface/CompressInterface.h"

class CFolderInStream: 
  public ISequentialInStream,
  public ICompressGetSubStreamSize,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CFolderInStream)
  COM_INTERFACE_ENTRY(ISequentialInStream)
  COM_INTERFACE_ENTRY(ICompressGetSubStreamSize)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFolderInStream)
DECLARE_NO_REGISTRY()

  CFolderInStream();

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

  STDMETHOD(GetSubStreamSize)(UINT64 subStream, UINT64 *value);
private:
  CComObjectNoLock<CInStreamWithCRC> *_inStreamWithHashSpec;
  CComPtr<ISequentialInStream> _inStreamWithHash;
  CComPtr<IUpdateCallBack> _updateCallback;

  bool _currentSizeIsDefined;
  UINT64 _currentSize;

  bool _fileIsOpen;
  UINT64 _filePos;

  const UINT32 *_fileIndices;
  UINT32 _numFiles;
  UINT32 _fileIndex;

  HRESULT OpenStream();
  HRESULT CloseStream();
  void AddDigest();
public:
  void Init(IUpdateCallBack *updateCallback, 
      const UINT32 *fileIndices, UINT32 numFiles);
  CRecordVector<UINT32> CRCs;
  CRecordVector<UINT64> Sizes;
  UINT64 GetFullSize() const
  {
    UINT64 size = 0;
    for (int i = 0; i < Sizes.Size(); i++)      
      size += Sizes[i];
    return size;
  }
};
  

#endif
