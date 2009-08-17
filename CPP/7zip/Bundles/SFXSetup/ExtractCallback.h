// ExtractCallback.h

#ifndef __EXTRACT_CALLBACK_H
#define __EXTRACT_CALLBACK_H

#include "resource.h"

#include "Windows/ResourceString.h"

#include "../../Archive/IArchive.h"

#include "../../Common/FileStreams.h"
#include "../../ICoder.h"

#ifndef _NO_PROGRESS
#include "../../UI/FileManager/ProgressDialog.h"
#endif
#include "../../UI/Common/ArchiveOpenCallback.h"

class CExtractCallbackImp:
  public IArchiveExtractCallback,
  public IOpenCallbackUI,
  public CMyUnknownImp
{
public:
  
  MY_UNKNOWN_IMP

  INTERFACE_IArchiveExtractCallback(;)
  INTERFACE_IOpenCallbackUI(;)

private:
  CMyComPtr<IInArchive> _archiveHandler;
  UString _directoryPath;
  UString _filePath;
  UString _diskFilePath;

  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME MTime;
    bool IsDir;
    UInt32 Attributes;
  } _processedFileInfo;

  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;

  UString _itemDefaultName;
  FILETIME _defaultMTime;
  UInt32 _defaultAttributes;

  void CreateComplexDirectory(const UStringVector &dirPathParts);
public:
  #ifndef _NO_PROGRESS
  CProgressDialog ProgressDialog;
  #endif

  bool _isCorrupt;
  UString _message;

  void Init(IInArchive *archiveHandler,
    const UString &directoryPath,
    const UString &itemDefaultName,
    const FILETIME &defaultMTime,
    UInt32 defaultAttributes);

  #ifndef _NO_PROGRESS
  HRESULT StartProgressDialog(const UString &title, NWindows::CThread &thread)
  {
    ProgressDialog.Create(title, thread, 0);
    {
      #ifdef LANG
      ProgressDialog.SetText(LangLoadString(IDS_PROGRESS_EXTRACTING, 0x02000890));
      #else
      ProgressDialog.SetText(NWindows::MyLoadStringW(IDS_PROGRESS_EXTRACTING));
      #endif
    }

    ProgressDialog.Show(SW_SHOWNORMAL);
    return S_OK;
  }
  virtual ~CExtractCallbackImp() { ProgressDialog.Destroy(); }
  #endif

};

#endif
