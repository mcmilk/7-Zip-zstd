// ExtractCallbackConsole.h

#include "StdAfx.h"

#include "ExtractCallbackConsole.h"
#include "UserInputUtils.h"

#include "ConsoleClose.h"
#include "Common/StdOutStream.h"
#include "Common/StdInStream.h"
#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/FileDir.h"
#include "Windows/FileFind.h"
#include "Windows/Time.h"
#include "Windows/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/Error.h"

#include "Windows/PropVariantConversions.h"

#include "../../Common/FilePathAutoRename.h"

#include "../Common/ExtractingFilePath.h"

using namespace NWindows;
using namespace NFile;
using namespace NDirectory;

static const char *kTestingString    =  "Testing     ";
static const char *kExtractingString =  "Extracting  ";
static const char *kSkippingString   =  "Skipping    ";

static const char *kCantAutoRename = "can not create file with auto name\n";
static const char *kCantRenameFile = "can not rename existing file\n";
static const char *kCantDeleteOutputFile = "can not delete output file ";
static const char *kError = "ERROR: ";
static const char *kMemoryExceptionMessage = "Can't allocate required memory!";
;


static const char *kProcessing = "Processing archive: ";
static const char *kEverythingIsOk = "Everything is Ok";
static const char *kNoFiles = "No files to process";

static const char *kEnterPassword = "Enter password:";

static const char *kUnsupportedMethod = "Unsupported Method";
static const char *kCRCFailed = "CRC Failed";
static const char *kDataError = "Data Error";
static const char *kUnknownError = "Unknown Error";

STDMETHODIMP CExtractCallbackConsole::SetTotal(UInt64 size)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::SetCompleted(const UInt64 *completeValue)
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    Int32 *answer)
{
  g_StdErr << "file " << existName << 
    "\nalready exists. Overwrite with " << endl;
  g_StdErr << newName;
  
  NUserAnswerMode::EEnum overwriteAnswer = ScanUserYesNoAllQuit();
  
  switch(overwriteAnswer)
  {
    case NUserAnswerMode::kQuit:
      return E_ABORT;
    case NUserAnswerMode::kNo:
      *answer = NOverwriteAnswer::kNo;
      break;
    case NUserAnswerMode::kNoAll:
      *answer = NOverwriteAnswer::kNoToAll;
      break;
    case NUserAnswerMode::kYesAll:
      *answer = NOverwriteAnswer::kYesToAll;
      break;
    case NUserAnswerMode::kYes:
      *answer = NOverwriteAnswer::kYes;
      break;
    case NUserAnswerMode::kAutoRename:
      *answer = NOverwriteAnswer::kAutoRename;
      break;
    default:
      return E_FAIL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::PrepareOperation(const wchar_t *name, Int32 askExtractMode, const UInt64 *position)
{
  switch (askExtractMode)
  {
    case NArchive::NExtract::NAskMode::kExtract:
      g_StdErr << kExtractingString;
      break;
    case NArchive::NExtract::NAskMode::kTest:
      g_StdErr << kTestingString;
      break;
    case NArchive::NExtract::NAskMode::kSkip:
      g_StdErr << kSkippingString;
      break;
  };
  g_StdErr << name;
  if (position != 0)
    g_StdErr << " <" << *position << ">";
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::MessageError(const wchar_t *message)
{
  g_StdErr << message << endl;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::SetOperationResult(Int32 operationResult)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      NumFileErrors++;
      g_StdErr << "     ";
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          g_StdErr << kUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          g_StdErr << kCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          g_StdErr << kDataError;
          break;
        default:
          g_StdErr << kUnknownError;
      }
    }
  }
  g_StdErr << endl;
  return S_OK;
}

STDMETHODIMP CExtractCallbackConsole::CryptoGetTextPassword(BSTR *password)
{
  if (!PasswordIsDefined)
  {
    g_StdErr << endl << kEnterPassword;
    AString oemPassword = g_StdIn.ScanStringUntilNewLine();
    Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
    PasswordIsDefined = true;
  }
  CMyComBSTR tempName = Password;
  *password = tempName.Detach();
  return S_OK;
}

HRESULT CExtractCallbackConsole::BeforeOpen(const wchar_t *name)
{
  g_StdErr << endl << kProcessing << name << endl;
  return S_OK;
}

HRESULT CExtractCallbackConsole::OpenResult(const wchar_t *name, HRESULT result)
{
  g_StdErr << endl;
  if (result != S_OK)
  {
    g_StdErr << "Error: " << name << " is not supported archive" << endl;
    NumArchiveErrors++;
  }
  return S_OK;
}
  
HRESULT CExtractCallbackConsole::ThereAreNoFiles()
{
  g_StdErr << endl << kNoFiles << endl;
  return S_OK;
}

HRESULT CExtractCallbackConsole::ExtractResult(HRESULT result)
{
  if (result == S_OK)
    g_StdErr << endl << kEverythingIsOk << endl;
  if (result == S_OK)
    return result;
  if (result == E_ABORT)
    return result;
  g_StdErr << kError;
  if (result == E_OUTOFMEMORY)
    g_StdErr << kMemoryExceptionMessage;
  else
  {
    UString message;
    NError::MyFormatMessage(result, message);
    g_StdErr << message;
  }
  g_StdErr << endl;

  NumArchiveErrors++;
  return S_OK;
}

HRESULT CExtractCallbackConsole::SetPassword(const UString &password)
{
  PasswordIsDefined = true;
  Password = password;
  return S_OK;
}
