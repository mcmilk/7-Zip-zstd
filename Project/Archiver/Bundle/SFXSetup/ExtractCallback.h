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
#include "../../Resource/ProgressDialog/ProgressDialog.h"
#include "../../Explorer/MyMessages.h"

class CExtractCallBackImp: 
  public IExtractCallback200,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBackImp)
  COM_INTERFACE_ENTRY(IExtractCallback200)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallBackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallback200
  STDMETHOD(Extract)(UINT32 anIndex, ISequentialOutStream **anOutStream, 
      INT32 anAskExtractMode);
  STDMETHOD(PrepareOperation)(INT32 anAskExtractMode);
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult);

private:
  CComPtr<IArchiveHandler200> m_ArchiveHandler;
  CProgressDialog m_ProgressDialog;
  CSysString m_DirectoryPath;

  CSysString m_FilePath;

  CSysString m_DiskFilePath;

  bool m_ExtractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    UINT32 Attributes;
  } m_ProcessedFileInfo;

  CComObjectNoLock<COutFileStream> *m_OutFileStreamSpec;
  CComPtr<ISequentialOutStream> m_OutFileStream;
  UINT m_CodePage;

  UString m_ItemDefaultName;
  FILETIME m_UTCLastWriteTimeDefault;
  UINT32 m_AttributesDefault;

  void CreateComplexDirectory(const UStringVector &aDirPathParts);
public:
  DWORD m_ThreadID;
  void Init(IArchiveHandler200 *anArchiveHandler,     
    const CSysString &aDirectoryPath, 
    const UString &anItemDefaultName,
    const FILETIME &anUTCLastWriteTimeDefault,
    UINT32 anAttributesDefault);

  UINT64 m_NumErrors;
  HRESULT StartProgressDialog()
  {
    m_ThreadID = GetCurrentThreadId();
    m_ProgressDialog.Create(0);
    {
      #ifdef LANG        
      m_ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890));
      #else
      m_ProgressDialog.SetText(NWindows::MyLoadString(IDS_PROGRESS_EXTRACTING));
      #endif
    }

    m_ProgressDialog.ShowWindow(SW_SHOWNORMAL);    
    // m_ProgressDialog.Start(m_ParentWindow, PROGDLG_MODAL | PROGDLG_AUTOTIME);
    return S_OK;
  }
  ~CExtractCallBackImp();
};

#endif
