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
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallBack
  STDMETHOD(Extract)(UINT32 anIndex, ISequentialOutStream **anOutStream, 
      INT32 anAskExtractMode);
  STDMETHOD(PrepareOperation)(INT32 anAskExtractMode);
  STDMETHOD(OperationResult)(INT32 aResultEOperationResult);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *aPassword);

private:
  CComPtr<IArchiveHandler200> m_ArchiveHandler;
  CComPtr<IExtractCallback2> m_ExtractCallback2;
  CComPtr<ICryptoGetTextPassword> m_CryptoGetTextPassword;
  CSysString m_DirectoryPath;
  NExtractionMode::NPath::EEnum m_PathMode;
  NExtractionMode::NOverwrite::EEnum m_OverwriteMode;

  UString m_FilePath;

  CSysString m_DiskFilePath;

  // bool m_MessagesDialogWasCreated;
  CSysStringVector m_Messages;
  // CSysString m_CurrentFilePath;

  bool m_ExtractMode;
  struct CProcessedFileInfo
  {
    FILETIME UTCLastWriteTime;
    bool IsDirectory;
    UINT32 Attributes;
  } m_ProcessedFileInfo;


  CComObjectNoLock<COutFileStream> *m_OutFileStreamSpec;
  CComPtr<ISequentialOutStream> m_OutFileStream;
  UStringVector m_RemovePathParts;
  // CProgressBox *m_ProgressBox;
  UINT m_CodePage;

  UString m_ItemDefaultName;
  FILETIME m_UTCLastWriteTimeDefault;
  UINT32 m_AttributesDefault;

  bool m_PasswordIsDefined;
  UString m_Password;


  void CreateComplexDirectory(const UStringVector &aDirPathParts);
  /*
  void GetPropertyValue(LPITEMIDLIST anItemIDList, PROPID aPropId, 
      PROPVARIANT *aValue);
  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  */
  void AddErrorMessage(LPCTSTR aMessage);
public:
  // CProgressDialog m_ProcessDialog;
  ~CExtractCallBack200Imp();
  void Init(
      IArchiveHandler200 *anArchiveHandler, 
      IExtractCallback2 *anExtractCallback2,
      const CSysString &aDirectoryPath,
      NExtractionMode::NPath::EEnum aPathMode,
      NExtractionMode::NOverwrite::EEnum anOverwriteMode,
      const UStringVector &aRemovePathParts,
      // CProgressBox *aProgressBox, 
      UINT aCodePage, 
      const UString &anItemDefaultName,
      const FILETIME &anUTCLastWriteTimeDefault, UINT32 anAttributesDefault,
      bool aPasswordIsDefined, const UString &aPassword);

  UINT64 m_NumErrors;
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

HRESULT ExtractArchive(HWND aParentWindow, const CSysString &aFileName);

#endif
