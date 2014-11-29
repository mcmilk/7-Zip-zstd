// ExtractCallbackConsole.h

#ifndef __EXTRACTCALLBACKCONSOLE_H
#define __EXTRACTCALLBACKCONSOLE_H

#include "../../../Common/MyString.h"
#include "../../../Common/StdOutStream.h"

#include "../../Common/FileStreams.h"

#include "../../IPassword.h"

#include "../../Archive/IArchive.h"

#include "../Common/ArchiveExtractCallback.h"

class CExtractCallbackConsole:
  public IExtractCallbackUI,
  #ifndef _NO_CRYPTO
  public ICryptoGetTextPassword,
  #endif
  public CMyUnknownImp
{
public:
  MY_QUERYINTERFACE_BEGIN2(IFolderArchiveExtractCallback)
  #ifndef _NO_CRYPTO
  MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
  #endif
  MY_QUERYINTERFACE_END
  MY_ADDREF_RELEASE

  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  // IFolderArchiveExtractCallback
  STDMETHOD(AskOverwrite)(
      const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
      const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
      Int32 *answer);
  STDMETHOD (PrepareOperation)(const wchar_t *name, bool isFolder, Int32 askExtractMode, const UInt64 *position);

  STDMETHOD(MessageError)(const wchar_t *message);
  STDMETHOD(SetOperationResult)(Int32 operationResult, bool encrypted);

  HRESULT BeforeOpen(const wchar_t *name);
  HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted);
  HRESULT SetError(int level, const wchar_t *name,
        UInt32 errorFlags, const wchar_t *errors,
        UInt32 warningFlags, const wchar_t *warnings);

  HRESULT ThereAreNoFiles();
  HRESULT ExtractResult(HRESULT result);
  HRESULT OpenTypeWarning(const wchar_t *name, const wchar_t *okType, const wchar_t *errorType);

 
  #ifndef _NO_CRYPTO
  HRESULT SetPassword(const UString &password);
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  bool PasswordIsDefined;
  UString Password;

  #endif
  
  UInt64 NumTryArcs;
  bool ThereIsErrorInCurrent;
  bool ThereIsWarningInCurrent;

  UInt64 NumCantOpenArcs;
  UInt64 NumOkArcs;
  UInt64 NumArcsWithError;
  UInt64 NumArcsWithWarnings;

  UInt64 NumProblemArcsLevs;
  UInt64 NumOpenArcErrors;
  UInt64 NumOpenArcWarnings;
  
  UInt64 NumFileErrors;
  UInt64 NumFileErrorsInCurrent;

  CStdOutStream *OutStream;

  void Init()
  {
    NumTryArcs = 0;
    NumOkArcs = 0;
    NumCantOpenArcs = 0;
    NumArcsWithError = 0;
    NumArcsWithWarnings = 0;

    NumOpenArcErrors = 0;
    NumOpenArcWarnings = 0;
    NumFileErrors = 0;
    NumFileErrorsInCurrent = 0;
  }

};

#endif
