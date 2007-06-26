// MainAr.cpp

#include "StdAfx.h"

// #include <locale.h>

#include "Windows/Error.h"

#include "Common/StdOutStream.h"
#include "Common/NewHandler.h"
#include "Common/MyException.h"
#include "Common/StringConvert.h"

#include "../Common/ExitCode.h"
#include "../Common/ArchiveCommandLine.h"
#include "ConsoleClose.h"

using namespace NWindows;

CStdOutStream *g_StdStream = 0;

#ifdef _WIN32
#ifndef _UNICODE
bool g_IsNT = false;
#endif
#if !defined(_UNICODE) || !defined(_WIN64)
static inline bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif
#endif

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
  g_StdStream = &g_StdOut;
  #ifdef _WIN32
  
  #ifdef _UNICODE
  #ifndef _WIN64
  if (!IsItWindowsNT())
  {
    (*g_StdStream) << "This program requires Windows NT/2000/XP/2003/Vista";
    return NExitCode::kFatalError;
  }
  #endif
  #else
  g_IsNT = IsItWindowsNT();
  #endif
  
  #endif

  // setlocale(LC_COLLATE, ".OCP");
  NConsoleClose::CCtrlHandlerSetter ctrlHandlerSetter;
  int res = 0;
  try
  {
    res = Main2(
#ifndef _WIN32
      numArguments, arguments
#endif
    );
  }
  catch(const CNewException &)
  {
    (*g_StdStream) << kMemoryExceptionMessage;
    return (NExitCode::kMemoryError);
  }
  catch(const NConsoleClose::CCtrlBreakException &)
  {
    (*g_StdStream) << endl << kUserBreak;
    return (NExitCode::kUserBreak);
  }
  catch(const CArchiveCommandLineException &e)
  {
    (*g_StdStream) << kExceptionErrorMessage << e << endl;
    return (NExitCode::kUserError);
  }
  catch(const CSystemException &systemError)
  {
    if (systemError.ErrorCode == E_OUTOFMEMORY)
    {
      (*g_StdStream) << kMemoryExceptionMessage;
      return (NExitCode::kMemoryError);
    }
    if (systemError.ErrorCode == E_ABORT)
    {
      (*g_StdStream) << endl << kUserBreak;
      return (NExitCode::kUserBreak);
    }
    UString message;
    NError::MyFormatMessage(systemError.ErrorCode, message);
    (*g_StdStream) << endl << endl << "System error:" << endl << 
        message << endl;
    return (NExitCode::kFatalError);
  }
  catch(NExitCode::EEnum &exitCode)
  {
    (*g_StdStream) << kInternalExceptionMessage << exitCode << endl;
    return (exitCode);
  }
  /*
  catch(const NExitCode::CMultipleErrors &multipleErrors)
  {
    (*g_StdStream) << endl << multipleErrors.NumErrors << " errors" << endl;
    return (NExitCode::kFatalError);
  }
  */
  catch(const UString &s)
  {
    (*g_StdStream) << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(const AString &s)
  {
    (*g_StdStream) << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(const char *s)
  {
    (*g_StdStream) << kExceptionErrorMessage << s << endl;
    return (NExitCode::kFatalError);
  }
  catch(int t)
  {
    (*g_StdStream) << kInternalExceptionMessage << t << endl;
    return (NExitCode::kFatalError);
  }
  catch(...)
  {
    (*g_StdStream) << kUnknownExceptionMessage;
    return (NExitCode::kFatalError);
  }
  return  res;
}
