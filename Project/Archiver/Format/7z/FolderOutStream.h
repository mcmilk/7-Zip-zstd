// 7z/FolderOutStream.h

#pragma once

#ifndef __7Z_FOLDEROUTSTREAM_H
#define __7Z_FOLDEROUTSTREAM_H

#include "ItemInfo.h"
#include "Header.h"

#include "Interface/IInOutStreams.h"
#include "../../Common/IArchiveHandler2.h"
#include "ItemInfoUtils.h"
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

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:

  CComObjectNoLock<COutStreamWithCRC> *m_OutStreamWithHashSpec;
  CComPtr<ISequentialOutStream> m_OutStreamWithHash;
  NArchive::N7z::CArchiveDatabaseEx *m_ArchiveDatabase;
  const CBoolVector *m_ExtractStatuses;
  UINT32 m_StartIndex;
  int m_CurrentIndex;
  UINT64 m_CurrentDataPos;
  CComPtr<IExtractCallback200> m_ExtractCallBack;
  bool m_TestMode;

  bool m_FileIsOpen;
  UINT64 m_FilePos;

  HRESULT OpenFile();
  HRESULT WriteEmptyFiles();
public:
  HRESULT Init(
      NArchive::N7z::CArchiveDatabaseEx *anArchiveDatabase,
      UINT32 aStartIndex,
      const CBoolVector *anExtractStatuses, 
      IExtractCallback200 *anExtractCallBack,
      bool aTestMode);
  HRESULT FlushCorrupted();
  HRESULT WasWritingFinished();
};

#endif
