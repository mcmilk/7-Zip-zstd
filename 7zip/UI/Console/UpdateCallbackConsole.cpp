// UpdateCallbackConsole.cpp

#include "StdAfx.h"

#include "UpdateCallbackConsole.h"

#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"
#include "Common/Defs.h"

#include "Windows/PropVariant.h"
#include "Windows/Error.h"

#include "ConsoleClose.h"

using namespace NWindows;


static const char *kCreatingArchiveMessage = "Creating archive ";
static const char *kUpdatingArchiveMessage = "Updating archive ";
static const char *kScanningMessage = "Scanning";
static const char *kNoFilesScannedMessage = "No files scanned";
static const char *kTotalFilesAddedMessage = "Total files added to archive: ";

HRESULT CUpdateCallbackConsole::OpenResult(const wchar_t *name, HRESULT result)
{
  g_StdErr << endl;
  if (result != S_OK)
  {
    g_StdErr << "Error: " << name << " is not supported archive" << endl;
  }
  return S_OK;
}

HRESULT CUpdateCallbackConsole::StartScanning()
{
  g_StdErr << kScanningMessage;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::FinishScanning()
{
  g_StdErr << endl << endl;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::StartArchive(const wchar_t *name, bool updating)
{
  if(updating)
    g_StdErr << kUpdatingArchiveMessage;
  else
    g_StdErr << kCreatingArchiveMessage; 
  if (name != 0)
    g_StdErr << name;
  else
    g_StdErr << "StdOut";
  g_StdErr << endl << endl;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::FinishArchive()
{
  g_StdErr << endl;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::CheckBreak()
{
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::Finilize()
{
  if (m_NeedBeClosed)
  {
    if (EnablePercents)
    {
      m_PercentPrinter.ClosePrint();
      m_PercentCanBePrint = false;
    }
    if (!StdOutMode)
      m_PercentPrinter.PrintNewLine();
    m_NeedBeClosed = false;
  }
  return S_OK;
}

HRESULT CUpdateCallbackConsole::SetTotal(UInt64 size)
{
  if (EnablePercents)
    m_PercentPrinter.SetTotal(size);
  return S_OK;
}

HRESULT CUpdateCallbackConsole::SetCompleted(const UInt64 *completeValue)
{
  if (completeValue != NULL)
  {
    if (EnablePercents)
    {
      m_PercentPrinter.SetRatio(*completeValue);
      if (m_PercentCanBePrint)
        m_PercentPrinter.PrintRatio();
    }
  }
  if (NConsoleClose::TestBreakSignal())
    return E_ABORT;
  return S_OK;
}

HRESULT CUpdateCallbackConsole::GetStream(const wchar_t *name, bool isAnti)
{
  if (StdOutMode)
    return S_OK;
  if(isAnti)
    m_PercentPrinter.PrintString("Anti item    ");
  else
    m_PercentPrinter.PrintString("Compressing  ");
  m_PercentPrinter.PrintString(name);
  if (EnablePercents)
  {
    m_PercentCanBePrint = true;
    m_PercentPrinter.PreparePrint();
    m_PercentPrinter.RePrintRatio();
  }
  return S_OK;
}

HRESULT CUpdateCallbackConsole::OpenFileError(const wchar_t *name, DWORD systemError)
{
  FailedCodes.Add(systemError);
  FailedFiles.Add(name);
  // if (systemError == ERROR_SHARING_VIOLATION)
  {
    m_PercentPrinter.ClosePrint();
    m_PercentPrinter.PrintNewLine();
    m_PercentPrinter.PrintString("WARNING: ");
    m_PercentPrinter.PrintString(NError::MyFormatMessageW(systemError));
    return S_FALSE;
  }
  return systemError;
}

HRESULT CUpdateCallbackConsole::SetOperationResult(Int32 operationResult)
{
  m_NeedBeClosed = true;
  return S_OK;  
}

HRESULT CUpdateCallbackConsole::CryptoGetTextPassword2(Int32 *passwordIsDefined, BSTR *password)
{
  if (!PasswordIsDefined) 
  {
    if (AskPassword)
    {
      g_StdErr << "\nEnter password:";
      AString oemPassword = g_StdIn.ScanStringUntilNewLine();
      Password = MultiByteToUnicodeString(oemPassword, CP_OEMCP); 
      PasswordIsDefined = true;
    }
  }
  *passwordIsDefined = BoolToInt(PasswordIsDefined);
  CMyComBSTR tempName(Password);
  *password = tempName.Detach();
  return S_OK;
}
