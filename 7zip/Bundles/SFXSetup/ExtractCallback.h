// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "resource.h"

#include "Common/String.h"
#include "Windows/ResourceString.h"

#include "../../Archive/IArchive.h"

#include "../../Common/FileStreams.h"
// #include "../../Common/ZipSettings.h"
#include "../../ICoder.h"

#ifndef _NO_PROGRESS
#include "../../FileManager/Resource/ProgressDialog/ProgressDialog.h"
#endif

// #include "../../Explorer/MyMessages.h"

class CExtractCallbackImp: 
  public IArchiveExtractCallback,
  public CMyUnknownImp
{
public:
  
  MY_UNKNOWN_IMP

  // IProgress
  STDMETHOD(SetTotal)(UINT64 size);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallback
  STDMETHOD(GetStream)(UINT32 index, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

private:
  CMyComPtr<IInArchive> _archiveHandler;
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

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;
  UINT _codePage;

  UString _itemDefaultName;
  FILETIME _utcLastWriteTimeDefault;
  UINT32 _attributesDefault;

  void CreateComplexDirectory(const UStringVector &dirPathParts);
public:
  #ifndef _NO_PROGRESS
  CProgressDialog ProgressDialog;
  #endif

  #ifdef _SILENT
  CSysString _message;
  #endif



  void Init(IInArchive *archiveHandler,     
    const CSysString &directoryPath, 
    const UString &itemDefaultName,
    const FILETIME &utcLastWriteTimeDefault,
    UINT32 attributesDefault);

  UINT64 _numErrors;

  #ifndef _NO_PROGRESS
  HRESULT StartProgressDialog(const CSysString &title)
  {
    ProgressDialog.Create(title, 0);
    {
      #ifdef LANG        
      ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890));
      #else
      ProgressDialog.SetText(NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING));
      #endif
    }

    ProgressDialog.Show(SW_SHOWNORMAL);    
    // _progressDialog.Start(m_ParentWindow, PROGDLG_MODAL | PROGDLG_AUTOTIME);
    return S_OK;
  }
  ~CExtractCallbackImp() { ProgressDialog.Destroy(); }
  #endif

};

#endif
