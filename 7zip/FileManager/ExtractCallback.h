// ExtractCallback.h

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../UI/Agent/IFolderArchive.h"
#include "Common/String.h"

#ifdef _SFX
#include "Resource/ProgressDialog/ProgressDialog.h"
#else
#include "Resource/ProgressDialog2/ProgressDialog.h"
#endif

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

#ifndef _NO_CRYPTO
#include "../IPassword.h"
#endif
#include "Common/MyCom.h"
#include "IFolder.h"

class CExtractCallbackImp: 
  public IExtractCallbackUI,
  public IFolderOperationsExtractCallback,
  #ifndef _NO_CRYPTO
  public ICryptoGetTextPassword,
  #endif
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP3(
      IFolderArchiveExtractCallback,
      IFolderOperationsExtractCallback,
      ICryptoGetTextPassword
  )

  // IProgress
  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IFolderArchiveExtractCallback
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *name, Int32 askExtractMode, const UInt64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(Int32 operationResult);

  // IExtractCallbackUI
  
  HRESULT BeforeOpen(const wchar_t *name);
  HRESULT OpenResult(const wchar_t *name, HRESULT result);
  HRESULT ThereAreNoFiles();
  HRESULT ExtractResult(HRESULT result);

  #ifndef _NO_CRYPTO
  HRESULT SetPassword(const UString &password);
  #endif

  // IFolderOperationsExtractCallback
  STDMETHOD(AskWrite)(
      const wchar_t *srcPath, 
      Int32 srcIsFolder, 
      const FILETIME *srcTime, 
      const UInt64 *srcSize,
      const wchar_t *destPathRequest, 
      BSTR *destPathResult, 
      Int32 *writeAnswer);
  STDMETHOD(ShowMessage)(const wchar_t *message);

  // ICryptoGetTextPassword
  #ifndef _NO_CRYPTO
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
  #endif

private:
  // CSysString _directoryPath;
  // CSysString m_DiskFilePath;
  // bool _extractMode;
  UString _currentArchivePath;
  bool _needWriteArchivePath;

  UString _currentFilePath;

  // void CreateComplexDirectory(const UStringVector &aDirPathParts);

  void AddErrorMessage(LPCWSTR message);
public:
  CProgressDialog ProgressDialog;
  CSysStringVector Messages;
  HWND ParentWindow;
  INT_PTR StartProgressDialog(const UString &title)
  {
    return ProgressDialog.Create(title, ParentWindow);
  }
  UInt32 NumArchiveErrors;
  NExtract::NOverwriteMode::EEnum OverwriteMode;

  #ifndef _NO_CRYPTO
  bool PasswordIsDefined;
  UString Password;
  #endif

  CExtractCallbackImp(): 
    #ifndef _NO_CRYPTO
    PasswordIsDefined(false),
    #endif
    OverwriteMode(NExtract::NOverwriteMode::kAskBefore),
    ParentWindow(0)
    {}
   
  ~CExtractCallbackImp();
  void Init();
};

#endif
