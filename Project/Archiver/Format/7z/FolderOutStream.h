// 7z/FolderOutStream.h

#pragma once

#ifndef __7Z_FOLDEROUTSTREAM_H
#define __7Z_FOLDEROUTSTREAM_H

#include "ItemInfo.h"

#include "Interface/IInOutStreams.h"
#include "../Common/ArchiveInterface.h"
#include "../Common/OutStreamWithCRC.h"

#include "InEngine.h"

class CFolderOutStream: 
  public ISequentialOutStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CFolderOutStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CFolderOutStream)
DECLARE_NO_REGISTRY()

  CFolderOutStream();

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
private:

  CComObjectNoLock<COutStreamWithCRC> *_outStreamWithHashSpec;
  CComPtr<ISequentialOutStream> _outStreamWithHash;
  NArchive::N7z::CArchiveDatabaseEx *_archiveDatabase;
  const CBoolVector *_extractStatuses;
  UINT32 _startIndex;
  int _currentIndex;
  // UINT64 _currentDataPos;
  CComPtr<IArchiveExtractCallback> _extractCallback;
  bool _testMode;

  bool _fileIsOpen;
  UINT64 _filePos;

  HRESULT OpenFile();
  HRESULT WriteEmptyFiles();
public:
  HRESULT Init(
      NArchive::N7z::CArchiveDatabaseEx *archiveDatabase,
      UINT32 startIndex,
      const CBoolVector *extractStatuses, 
      IArchiveExtractCallback *extractCallback,
      bool testMode);
  HRESULT FlushCorrupted(INT32 resultEOperationResult);
  HRESULT WasWritingFinished();
};

#endif
