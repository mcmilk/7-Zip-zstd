// MainAr.cpp

#include "StdAfx.h"

// #include <locale.h>

#include "Windows/Error.h"

#include "Common/StdOutStream.h"
#include "Common/NewHandler.h"
#include "Common/Exception.h"
#include "Common/StringConvert.h"

#include "../Common/ExitCode.h"
#include "ConsoleClose.h"

using namespace NWindows;

extern int Main2(
  #ifndef _WIN32  
  int numArguments, const char *arguments[]
  #endif
);

static const char *kExceptionErrorMessage = "\n\nError:\n";
static const char *kUserBreak  = "\nBreak signaled\n";

static const char *kMemoryExceptionMessage = "\n\nERROR: Can't allocate required memory!\n";
static const char *kUnknownExceptionMessage = "\n\nUnknown Error\n";
static const char *kInternalExceptionMessage = "\n\nInternal Error #";

#ifdef UNICODE
static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

int 
#ifdef _MSC_VER
__cdecl 
#endif
main
(
#ifndef _WIN32  
int numArguments, const char *arguments[]
#endif
)
{
  #ifdef UNICODE
  if (!IsItWindowsNT())
  {
    g_StdErr << "This program requires Windows NT/2000/XP/2003";
    return NExitCode::kFatalError;
  }
  #endif
  // setlocale(LC_COLLATE, ".OCP");
  NConsoleClose::CCtrlHandlerSetter ctrlHandlerSetter;
  try
  {
    return Main2(
#ifndef _WIN32
      numArguments, arguments
#endif
    );
  }
  catch(const CNewException &)
  {
    g_StdErr << kMemoryExceptionMessage;
    return (NExitCode::kMemoryError);
  }
  catch(const NConsoleClose::CCtrlBreakException &)
  {
    g_StdErr << endl << kUserBreak;
    return (NExitCode::kUserBreak);
  }
  catch(const CSystemException &systemError)
  {
    if (systemError.ErrorCode == E_OUTOFMEMORY)
    {
      g_StdErr << kMemoryExceptionMessage;
      return (NExitCode::kMemoryError);
    }
    if (systemError.ErrorCode == E_ABORT)
    {
      g_StdErr << endl << kUserBreak;
      return (NExitCode::kUserBreak);
    }
    UString message;
    NError::MyFormatMessage(systemError.ErrorCode, message);
    g_StdErr << endl << endl << "System error:" << endl << 
        message << endl;
    return (NExitCode::kFatalError);
  }
  catch(NExitCode::EEnum &exitCode)
  {
    g_StdErr << kInternalExceptionMessage << exitCode << endl;
    return (exitCode);
  }
  /*
  catch(const NExitCode::CMultipleErrors &multipleErrors)
  {
    g_StdErr << endl << multipleErrors.NumErrors << " errors" << endl;
    return (NExitCode::kFatalError);
  }
  */
  catch(const UString &s)
  {
    g_StdErr << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(const AString &s)
  {
    g_StdErr << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(const char *s)
  {
    g_StdErr << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(int t)
  {
    g_StdErr << kInternalExceptionMessage << t << endl;
    return (NExitCode::kFatalError);
  }
  catch(...)
  {
    g_StdErr << kUnknownExceptionMessage;
    return (NExitCode::kFatalError);
  }
}
