// UpdateCallbackConsole.h

#ifndef __UPDATECALLBACKCONSOLE_H
#define __UPDATECALLBACKCONSOLE_H

#include "Common/MyString.h"
#include "Common/StdOutStream.h"
#include "PercentPrinter.h"
#include "../Common/Update.h"

class CUpdateCallbackConsole: public IUpdateCallbackUI2
{
  CPercentPrinter m_PercentPrinter;
  bool m_NeedBeClosed;
  bool m_NeedNewLine;

  bool m_WarningsMode;

  CStdOutStream *OutStream;
public:
  bool EnablePercents;
  bool StdOutMode;

  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;


  CUpdateCallbackConsole(): 
      m_PercentPrinter(1 << 16),
      PasswordIsDefined(false),
      AskPassword(false),
      StdOutMode(false),
      EnablePercents(true),
      m_WarningsMode(false)
      {}
  
  ~CUpdateCallbackConsole() { Finilize(); }
  void Init(CStdOutStream *outStream)
  {
    m_NeedBeClosed = false;
    m_NeedNewLine = false;
    FailedFiles.Clear();
    FailedCodes.Clear();
    OutStream = outStream;
    m_PercentPrinter.OutStream = outStream;
  }

  HRESULT OpenResult(const wchar_t *name, HRESULT result);

  HRESULT StartScanning();
  HRESULT CanNotFindError(const wchar_t *name, DWORD systemError);
  HRESULT FinishScanning();

  HRESULT StartArchive(const wchar_t *name, bool updating);
  HRESULT FinishArchive();

  HRESULT CheckBreak();
  HRESULT Finilize();
  HRESULT SetTotal(UInt64 size);
  HRESULT SetCompleted(const UInt64 *completeValue);

  HRESULT GetStream(const wchar_t *name, bool isAnti);
  HRESULT OpenFileError(const wchar_t *name, DWORD systemError);
  HRESULT SetOperationResult(Int32 operationResult);
  HRESULT CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password);

  UStringVector FailedFiles;
  CRecordVector<HRESULT> FailedCodes;

  UStringVector CantFindFiles;
  CRecordVector<HRESULT> CantFindCodes;
};

#endif
