// 7z/FolderInStream.h

#pragma once

#ifndef __7Z_FOLDERINSTREAM_H
#define __7Z_FOLDERINSTREAM_H

#include "7zItem.h"
#include "7zHeader.h"

#include "../IArchive.h"
#include "../Common/InStreamWithCRC.h"
#include "../../IStream.h"
#include "../../ICoder.h"

namespace NArchive {
namespace N7z {

class CFolderInStream: 
  public ISequentialInStream,
  public ICompressGetSubStreamSize,
  public CMyUnknownImp
{
public:

  MY_UNKNOWN_IMP1(ICompressGetSubStreamSize)

  CFolderInStream();

  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);

  STDMETHOD(GetSubStreamSize)(UINT64 subStream, UINT64 *value);
private:
  CInStreamWithCRC *_inStreamWithHashSpec;
  CMyComPtr<ISequentialInStream> _inStreamWithHash;
  CMyComPtr<IArchiveUpdateCallback> _updateCallback;

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
  void Init(IArchiveUpdateCallback *updateCallback, 
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

}}

#endif
