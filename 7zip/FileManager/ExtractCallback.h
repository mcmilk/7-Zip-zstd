// ExtractCallback.h

#pragma once

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../UI/Agent/IFolderArchive.h"
#include "Common/String.h"

#ifdef _SFX
#include "Resource/ProgressDialog/ProgressDialog.h"
#else
#include "Resource/ProgressDialog2/ProgressDialog.h"
#endif
// #include "resource.h"

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

#include "../IPassword.h"
#include "Common/MyCom.h"
#include "IFolder.h"

class CExtractCallbackImp: 
  public IFolderArchiveExtractCallback,
  public IFolderOperationsExtractCallback,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
      IFolderArchiveExtractCallback,
      IFolderOperationsExtractCallback,
      ICryptoGetTextPassword
  )

  // IProgress
  STDMETHOD(SetTotal)(UINT64 total);
  STDMETHOD(SetCompleted)(const UINT64 *completeValue);

  // IFolderArchiveExtractCallback
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UINT64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UINT64 *newSize,
      INT32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *name, INT32 askExtractMode);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(INT32 operationResult);

  // IFolderOperationsExtractCallback
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

  void AddErrorMessage(LPCTSTR message);
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
