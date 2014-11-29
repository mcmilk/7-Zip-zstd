// UpdateCallbackConsole.h

#ifndef __UPDATE_CALLBACK_CONSOLE_H
#define __UPDATE_CALLBACK_CONSOLE_H

#include "../../../Common/StdOutStream.h"

#include "../Common/Update.h"

#include "PercentPrinter.h"

class CCallbackConsoleBase
{
  bool m_WarningsMode;
protected:
  CPercentPrinter m_PercentPrinter;

  CStdOutStream *OutStream;
  HRESULT CanNotFindError_Base(const wchar_t *name, DWORD systemError);
public:
  bool EnablePercents;
  bool StdOutMode;

  CCallbackConsoleBase():
      m_PercentPrinter(1 << 16),
      StdOutMode(false),
      EnablePercents(true),
      m_WarningsMode(false)
      {}
  
  void Init(CStdOutStream *outStream)
  {
    FailedFiles.Clear();
    FailedCodes.Clear();
    OutStream = outStream;
    m_PercentPrinter.OutStream = outStream;
  }

  UStringVector FailedFiles;
  CRecordVector<HRESULT> FailedCodes;

  UStringVector CantFindFiles;
  CRecordVector<HRESULT> CantFindCodes;
};

class CUpdateCallbackConsole: public IUpdateCallbackUI2, public CCallbackConsoleBase
{
  bool m_NeedBeClosed;
  bool m_NeedNewLine;
public:
  #ifndef _NO_CRYPTO
  bool PasswordIsDefined;
  UString Password;
  bool AskPassword;
  #endif

  CUpdateCallbackConsole()
      #ifndef _NO_CRYPTO
      :
      PasswordIsDefined(false),
      AskPassword(false)
      #endif
      {}
  
  void Init(CStdOutStream *outStream)
  {
    m_NeedBeClosed = false;
    m_NeedNewLine = false;
    CCallbackConsoleBase::Init(outStream);
  }
  ~CUpdateCallbackConsole() { Finilize(); }
  INTERFACE_IUpdateCallbackUI2(;)
};

#endif
