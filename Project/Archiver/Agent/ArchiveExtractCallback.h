// ArchiveExtractCallback.h

#pragma once

#ifndef __ARCHIVEEXTRACTCALLBACK_H
#define __ARCHIVEEXTRACTCALLBACK_H

#include "../Format/Common/ArchiveInterface.h"
#include "../Common/FolderArchiveInterface.h"

#include "Common/String.h"

#include "Interface/FileStreams.h"
#include "Interface/CryptoInterface.h"

class CArchiveExtractCallback: 
  public IArchiveExtractCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CArchiveExtractCallback)
  COM_INTERFACE_ENTRY(IArchiveExtractCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CArchiveExtractCallback)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aize);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(GetStream)(UINT32 anIndex, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(SetOperationResult)(INT32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CComPtr<IInArchive> _archiveHandler;
  CComPtr<IFolderArchiveExtractCallback> _extractCallback2;
  CComPtr<ICryptoGetTextPassword> _cryptoGetTextPassword;
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


  CComObjectNoLock<COutFileStream> *_outFileStreamSpec;
  CComPtr<ISequentialOutStream> _outFileStream;
  UStringVector _removePathParts;
  UINT _codePage;

  UString _itemDefaultName;
  FILETIME _utcLastWriteTimeDefault;
  UINT32 _attributesDefault;

  // bool m_PasswordIsDefined;
  // UString m_Password;


  void CreateComplexDirectory(const UStringVector &dirPathParts);
  /*
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  */
  void AddErrorMessage(LPCTSTR message);
public:
  // CProgressDialog m_ProcessDialog;
  ~CArchiveExtractCallback();
  void Init(
      IInArchive *archiveHandler, 
      IFolderArchiveExtractCallback *extractCallback2,
      const CSysString &directoryPath,
      NExtractionMode::NPath::EEnum pathMode,
      NExtractionMode::NOverwrite::EEnum overwriteMode,
      const UStringVector &removePathParts,
      UINT codePage, 
      const UString &itemDefaultName,
      const FILETIME &utcLastWriteTimeDefault, UINT32 anAttributesDefault
      // bool aPasswordIsDefined, const UString &aPassword
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
