// 7zFolderOutStream.h

#pragma once

#ifndef __7Z_FOLDEROUTSTREAM_H
#define __7Z_FOLDEROUTSTREAM_H

#include "7zIn.h"

#include "../../IStream.h"
#include "../IArchive.h"
#include "../Common/OutStreamWithCRC.h"

namespace NArchive {
namespace N7z {

class CFolderOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP
  
  CFolderOutStream();

  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
private:

  COutStreamWithCRC *_outStreamWithHashSpec;
  CMyComPtr<ISequentialOutStream> _outStreamWithHash;
  CArchiveDatabaseEx *_archiveDatabase;
  const CBoolVector *_extractStatuses;
  UINT32 _startIndex;
  int _currentIndex;
  // UINT64 _currentDataPos;
  CMyComPtr<IArchiveExtractCallback> _extractCallback;
  bool _testMode;

  bool _fileIsOpen;
  UINT64 _filePos;

  HRESULT OpenFile();
  HRESULT WriteEmptyFiles();
public:
  HRESULT Init(
      CArchiveDatabaseEx *archiveDatabase,
      UINT32 startIndex,
      const CBoolVector *extractStatuses, 
      IArchiveExtractCallback *extractCallback,
      bool testMode);
  HRESULT FlushCorrupted(INT32 resultEOperationResult);
  HRESULT WasWritingFinished();
};

}}

#endif
