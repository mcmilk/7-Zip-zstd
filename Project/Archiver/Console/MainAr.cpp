// MainAr.cpp

#include "StdAfx.h"

// #include <locale.h>

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

static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

int __cdecl main(int aNumArguments, const char *anArguments[])
{
  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    g_StdOut << "This program requires Windows NT/2000/XP";
    return NExitCode::kFatalError;
  }
  #endif
  // setlocale(LC_COLLATE, ".OCP");
  int result=1;
  // CNewHandlerSetter aNewHandlerSetter;
  NCOM::CComInitializer comInitializer;
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
      return (NExitCode::kUserBreak);
    }
  }
  catch(const CNewException)
  {
    g_StdOut << kMemoryExceptionMessage;
    return (NExitCode::kMemoryError);
  }
  catch(const CSystemException &e)
  {
    g_StdOut << "System Error: " << (UINT64)(e.ErrorCode);
    return (NExitCode::kFatalError);
  }
  catch(NExitCode::EEnum &aExitCode)
  {
    g_StdOut << kInternalExceptionMessage << aExitCode << endl;
    return (aExitCode);
  }
  catch(const NExitCode::CSystemError &aSystemError)
  {
    CSysString aMessage;
    NError::MyFormatMessage(aSystemError.ErrorValue, aMessage);
    g_StdOut << endl << endl << "System error:" << endl << 
        SystemStringToOemString(aMessage) << endl;
    return (NExitCode::kFatalError);
  }
  catch(const NExitCode::CMultipleErrors &aMultipleErrors)
  {
    g_StdOut << endl << aMultipleErrors.NumErrors << " errors" << endl;
    return (NExitCode::kFatalError);
  }
  catch(const UString &aString)
  {
    g_StdOut << kExceptionErrorMessage << 
      UnicodeStringToMultiByte(aString, CP_OEMCP) << endl;
    return (NExitCode::kFatalError);
  }
  catch(const char *aString)
  {
    g_StdOut << kExceptionErrorMessage << aString << endl;
    return (NExitCode::kFatalError);
  }
  catch(int t)
  {
    g_StdOut << kInternalExceptionMessage << t << endl;
    return (NExitCode::kFatalError);
  }
  catch(...)
  {
    g_StdOut << kUnknownExceptionMessage;
    return (NExitCode::kFatalError);
  }
  return result;
}
