// ExtractCallbackConsole.h

#ifndef __EXTRACTCALLBACKCONSOLE_H
#define __EXTRACTCALLBACKCONSOLE_H

#include "Common/MyString.h"
#include "Common/StdOutStream.h"
#include "../../Common/FileStreams.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../Common/ArchiveExtractCallback.h"

class CExtractCallbackConsole: 
  public IExtractCallbackUI,
  public ICryptoGetTextPassword,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP2(IFolderArchiveExtractCallback, ICryptoGetTextPassword)

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

  // ICryptoGetTextPassword
  STDMETHOD(CryptoGetTextPassword)(BSTR *password);

  HRESULT BeforeOpen(const wchar_t *name);
  HRESULT OpenResult(const wchar_t *name, HRESULT result, bool encrypted);
  HRESULT ThereAreNoFiles();
  HRESULT ExtractResult(HRESULT result);

  HRESULT SetPassword(const UString &password);

public:
  bool PasswordIsDefined;
  UString Password;
  
  UInt64 NumArchives;
  UInt64 NumArchiveErrors;
  UInt64 NumFileErrors;
  UInt64 NumFileErrorsInCurrentArchive;

  CStdOutStream *OutStream;

  void Init()
  {
    NumArchives = 0;
    NumArchiveErrors = 0;
    NumFileErrors = 0;
    NumFileErrorsInCurrentArchive = 0;
  }

};

#endif
