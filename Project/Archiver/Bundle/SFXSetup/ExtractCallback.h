// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "resource.h"

#include "Common/String.h"
#include "Windows/ResourceString.h"

#include "../../Common/IArchiveHandler2.h"

#include "Interface/FileStreams.h"
#include "../../Common/ZipSettings.h"
#include "../../../Compress/Interface./CompressInterface.h"
#include "../../../FileManager/Resource/ProgressDialog/ProgressDialog.h"
#include "../../Explorer/MyMessages.h"

class CExtractCallbackImp: 
  public IExtractCallback200,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallbackImp)
  COM_INTERFACE_ENTRY(IExtractCallback200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallbackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallback200
  STDMETHOD(Extract)(UINT32 index, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(OperationResult)(INT32 resultEOperationResult);

private:
  CComPtr<IArchiveHandler200> _archiveHandler;
  CSysString _directoryPath;

  CSysString _filePath;

  CSysString _diskFilePath;

  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    UINT32 Attributes;
  } _processedFileInfo;

  CComObjectNoLock<COutFileStream> *_outFileStreamSpec;
  CComPtr<ISequentialOutStream> _outFileStream;
  UINT _codePage;

  UString _itemDefaultName;
  FILETIME _utcLastWriteTimeDefault;
  UINT32 _attributesDefault;

  void CreateComplexDirectory(const UStringVector &dirPathParts);
public:
  // DWORD _threadID;
  CProgressDialog _progressDialog;

  void Init(IArchiveHandler200 *archiveHandler,     
    const CSysString &directoryPath, 
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault);

  UINT64 _numErrors;

  HRESULT StartProgressDialog(const CSysString &title)
  {
    _progressDialog.Create(title, 0);
    // _threadID = GetCurrentThreadId();
    // _progressDialog.Create(0);
    {
      #ifdef LANG        
      _progressDialog.SetText(LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890));
      #else
      _progressDialog.SetText(NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING));
      #endif
    }

    _progressDialog.Show(SW_SHOWNORMAL);    
    // _progressDialog.Start(m_ParentWindow, PROGDLG_MODAL | PROGDLG_AUTOTIME);
    return S_OK;
  }

  ~CExtractCallbackImp();
};

#endif
