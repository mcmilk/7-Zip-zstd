// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../Archiver/Common/FolderArchiveInterface.h"
#include "Common/String.h"

#include "Resource/ProgressDialog/ProgressDialog.h"
// #include "resource.h"

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

#include "Interface/CryptoInterface.h"
#include "FolderInterface.h"

class CExtractCallbackImp: 
  public IFolderArchiveExtractCallback,
  public IFolderOperationsExtractCallback,
  public ICryptoGetTextPassword,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CExtractCallbackImp)
  COM_INTERFACE_ENTRY(IFolderArchiveExtractCallback)
  COM_INTERFACE_ENTRY(IFolderOperationsExtractCallback)
  COM_INTERFACE_ENTRY(ICryptoGetTextPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CExtractCallbackImp)

DECLARE_NO_REGISTRY()

  // IProgress
  STDMETHOD(SetTotal)(UINT64 aSize);
  STDMETHOD(SetCompleted)(const UINT64 *aCompleteValue);

  // IExtractCallBack2
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
      INT32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *aName, INT32 anAskExtractMode);

  STDMETHOD(MessageError)(const wchar_t *aMessage);
  STDMETHOD(SetOperationResult)(INT32 operationResult);

  STDMETHOD(AskWrite)(
      const wchar_t *srcPath, 
      INT32 srcIsFolder, 
      const FILETIME *srcTime, 
      const UINT64 *srcSize,
      const wchar_t *destPathRequest, 
      BSTR *destPathResult, 
      INT32 *writeAnswer);
  STDMETHOD(ShowMessage)(const wchar_t *message);

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

private:
  // CSysString _directoryPath;
  // CSysString m_DiskFilePath;
  // bool _extractMode;

  UString _currentFilePath;
  NExtractionMode::NOverwrite::EEnum _overwriteMode;

  bool _passwordIsDefined;
  UString _password;


  void CreateComplexDirectory(const UStringVector &aDirPathParts);

  bool IsEncrypted(LPITEMIDLIST anItemIDList);
  void AddErrorMessage(LPCTSTR aMessage);
public:
  CProgressDialog ProgressDialog;
  CSysStringVector _messages;
  HWND _parentWindow;
  UINT _fileCodePage;
  void StartProgressDialog(const CSysString &title)
  {
    ProgressDialog.Create(title, _parentWindow);
  }

  ~CExtractCallbackImp();
  void Init(NExtractionMode::NOverwrite::EEnum overwriteMode,
      bool passwordIsDefined, const UString &password);
  // void DestroyWindows();
};

#endif
