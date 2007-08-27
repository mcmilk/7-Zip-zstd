// ExtractCallback.h

#ifndef __EXTRACTCALLBACK_H
#define __EXTRACTCALLBACK_H

#include "../Agent/IFolderArchive.h"
#include "Common/MyString.h"

#ifdef _SFX
#include "ProgressDialog.h"
#else
#include "ProgressDialog2.h"
#endif

#include "Windows/ResourceString.h"

#ifdef LANG        
#include "LangUtils.h"
#endif

#ifndef _NO_CRYPTO
#include "../../IPassword.h"
#endif
#include "Common/MyCom.h"
#include "IFolder.h"

class CExtractCallbackImp: 
  public IExtractCallbackUI,
  public IFolderOperationsExtractCallback,
  // public IFolderArchiveExtractCallback, // mkultiple from IProgress
  #ifndef _SFX
  public ICompressProgressInfo,
  #endif
  #ifndef _NO_CRYPTO
  public ICryptoGetTextPassword,
  #endif
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IFolderOperationsExtractCallback)
  MY_QUERYINTERFACE_ENTRY(IFolderArchiveExtractCallback)
  #ifndef _SFX
  MY_QUERYINTERFACE_ENTRY(ICompressProgressInfo)
  #endif
  #ifndef _NO_CRYPTO
  MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  // IProgress
  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *value);

  #ifndef _SFX
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
  #endif

  // IFolderArchiveExtractCallback
  // STDMETHOD(SetTotalFiles)(UInt64 total);
  // STDMETHOD(SetCompletedFiles)(const UInt64 *value);
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *name, bool isFolder, Int32 askExtractMode, const UInt64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(Int32 operationResult, bool encrypted);

  // IExtractCallbackUI
  
  HRESULT BeforeOpen(const wchar_t *name);
  HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted);
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
  STDMETHOD(SetCurrentFilePath)(const wchar_t *filePath);
  STDMETHOD(SetNumFiles)(UInt64 numFiles);

  // ICryptoGetTextPassword
  #ifndef _NO_CRYPTO
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
  #endif

private:
  // bool _extractMode;
  UString _currentArchivePath;
  bool _needWriteArchivePath;

  UString _currentFilePath;
  bool _isFolder;

  // void CreateComplexDirectory(const UStringVector &aDirPathParts);

  HRESULT SetCurrentFilePath2(const wchar_t *filePath);
  void AddErrorMessage(LPCWSTR message);
public:
  CProgressDialog ProgressDialog;
  UStringVector Messages;
  bool ShowMessages;
  #ifndef _SFX
  UInt64 NumFolders;
  UInt64 NumFiles;
  bool NeedAddFile;
  #endif
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
    ParentWindow(0),
    ShowMessages(true)
    {}
   
  ~CExtractCallbackImp();
  void Init();
};

#endif
