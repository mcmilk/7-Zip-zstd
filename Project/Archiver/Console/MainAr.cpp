// MainAr.cpp

#include "StdAfx.h"

#include <locale.h>

#include "Windows/COM.h"
#include "Windows/Error.h"

#include "Common/StdOutStream.h"
#include "Common/NewHandler.h"
#include "Common/StringConvert.h"

#include "ConsoleCloseUtils.h"
#include "ArError.h"

using namespace NWindows;

extern int Main2(int aNumArguments, const char *anArguments[]);

static const char *kExceptionErrorMessage = "\n\nError:\n";
static const char *kUserBreak  = "\nBreak signaled\n";

static const char *kMemoryExceptionMessage = "\n\nMemory Error! Can't allocate!\n";
static const char *kUnknownExceptionMessage = "\n\nUnknown Error\n";
static const char *kInternalExceptionMessage = "\n\nInternal Error #";

static bool IsItWindowsNT()
{
  OSVERSIONINFO aVersionInfo;
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!::GetVersionEx(&aVersionInfo)) 
    return false;
  return (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

int main(int aNumArguments, const char *anArguments[])
{
  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    g_StdOut << "This program requires Windows NT/2000/XP";
    return NExitCode::kFatalError;
  }
  #endif
  setlocale(LC_COLLATE, ".OCP");
  int result=1;
  CNewHandlerSetter aNewHandlerSetter;
  NCOM::CComInitializer aComInitializer;
  try
  {
    NConsoleClose::CCtrlHandlerSetter aCtrlHandlerSetter;
    try
    {
      result = Main2(aNumArguments, anArguments);
    }
    catch(const NConsoleClose::CCtrlBreakException &)
    {
      g_StdOut << endl << kUserBreak;
      exit(NExitCode::kUserError);
    }
  }
  catch(const CNewException)
  {
    g_StdOut << kMemoryExceptionMessage;
    exit(NExitCode::kMemoryError);
  }
  catch(NExitCode::EEnum &aExitCode)
  {
    g_StdOut << kInternalExceptionMessage << aExitCode << endl;
    exit(aExitCode);
  }
  catch(const NExitCode::CSystemError &aSystemError)
  {
    CSysString aMessage;
    NError::MyFormatMessage(aSystemError.ErrorValue, aMessage);
    g_StdOut << endl << endl << "System error:" << endl << 
        SystemStringToOemString(aMessage) << endl;
    exit(NExitCode::kFatalError);
  }
  catch(const NExitCode::CMultipleErrors &aMultipleErrors)
  {
    g_StdOut << endl << aMultipleErrors.NumErrors << " errors" << endl;
    exit(NExitCode::kFatalError);
  }
  catch(const char *aString)
  {
    g_StdOut << kExceptionErrorMessage << aString << endl;
    exit(NExitCode::kFatalError);
  }
  catch(int t)
  {
    g_StdOut << kInternalExceptionMessage << t << endl;
    exit(NExitCode::kFatalError);
  }
  catch(...)
  {
    g_StdOut << kUnknownExceptionMessage;
    exit(NExitCode::kFatalError);
  }
  return result;
}
