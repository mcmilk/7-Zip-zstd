// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK200_H
#define __EXTRACTCALLBACK200_H

#include "../Format/Common/IArchiveHandler.h"
#include "../Common/IArchiveHandler2.h"

#include "Common/String.h"

// #include "../Common/ProgressBox.h"

// #include "ExtractDialog.h"
// #include "MessagesDialog.h"

#include "Interface/FileStreams.h"
// #include "../Common/ZipRegistry.h"
#include "../Format/Common/FormatCryptoInterface.h"

class CExtractCallBack200Imp: 
  public IExtractCallback200,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallBack200Imp)
  COM_INTERFACE_ENTRY(IExtractCallback200)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallBack200Imp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aize);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IExtractCallBack
  STDMETHOD(Extract)(UINT32 anIndex, ISequentialOutStream **outStream, 
      INT32 askExtractMode);
  STDMETHOD(PrepareOperation)(INT32 askExtractMode);
  STDMETHOD(OperationResult)(INT32 resultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CComPtr<IArchiveHandler200> _archiveHandler;
  CComPtr<IExtractCallback2> _extractCallback2;
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
  ~CExtractCallBack200Imp();
  void Init(
      IArchiveHandler200 *archiveHandler, 
      IExtractCallback2 *extractCallback2,
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
