// ArchiveExtractCallback.h

#pragma once

#ifndef __ARCHIVEEXTRACTCALLBACK_H
#define __ARCHIVEEXTRACTCALLBACK_H

#include "../../Archive/IArchive.h"
#include "IFolderArchive.h"

#include "Common/String.h"
#include "Common/MyCom.h"

#include "../../Common/FileStreams.h"
#include "../../IPassword.h"

class CArchiveExtractCallback: 
  public IArchiveExtractCallback,
  // public IArchiveVolumeExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ICryptoGetTextPassword)
  // COM_INTERFACE_ENTRY(IArchiveVolumeExtractCallback)

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aize);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(GetStream)(UINT32 anIndex, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

  // IArchiveVolumeExtractCallback
  // STDMETHOD(GetInStream)(const wchar_t *name, ISequentialInStream **inStream);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CMyComPtr<IInArchive> _archiveHandler;
  CMyComPtr<IFolderArchiveExtractCallback> _extractCallback2;
  CMyComPtr<ICryptoGetTextPassword> _cryptoGetTextPassword;
  CSysString _directoryPath;
  NExtractionMode::NPath::EEnum _pathMode;
  NExtractionMode::NOverwrite::EEnum _overwriteMode;

  UString _filePath;

  CSysString _diskFilePath;

  CSysStringVector _messages;

  bool _extractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    bool AttributesAreDefined;
    UINT32 Attributes;
  } _processedFileInfo;


  COutFileStream *_outFileStreamSpec;
  CMyComPtr<ISequentialOutStream> _outFileStream;
  UStringVector _removePathParts;
  UINT _codePage;

  UString _itemDefaultName;
  FILETIME _utcLastWriteTimeDefault;
  UINT32 _attributesDefault;

  // bool m_PasswordIsDefined;
  // UString m_Password;

  
  // CSysString _srcDirectoryPrefix;

  void CreateComplexDirectory(const UStringVector &dirPathParts);
  /*
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  */
  void AddErrorMessage(LPCTSTR message);
public:
  // CProgressDialog m_ProcessDialog;
  void Init(
      IInArchive *archiveHandler, 
      IFolderArchiveExtractCallback *extractCallback2,
      const CSysString &directoryPath,
      NExtractionMode::NPath::EEnum pathMode,
      NExtractionMode::NOverwrite::EEnum overwriteMode,
      const UStringVector &removePathParts,
      UINT codePage, 
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, 
      UINT32 anAttributesDefault
      // bool aPasswordIsDefined, const UString &aPassword
      // CSysString srcDirectoryPrefix
      );

  UINT64 _numErrors;
};

/*
namespace NExtractResult
{
  enum EEnum
  {
    kSuccess,
    kError,
    kNotArchive,
    kCanNotCreateInstance,
    kUserCancel
  };
}
*/

// HRESULT ExtractArchive(HWND aParentWindow, const CSysString &aFileName);

#endif
