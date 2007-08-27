// GUI.cpp

#include "StdAfx.h"

#include <initguid.h>

extern "C" 
{ 
  #include "../../../../C/Alloc.h"
}

#include "Common/NewHandler.h"
#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"
#include "Common/MyException.h"

#include "Windows/COM.h"
#include "Windows/FileMapping.h"
#include "Windows/FileDir.h"
#include "Windows/Synchronization.h"
#include "Windows/Error.h"
#include "Windows/FileName.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#endif

#include "../../IStream.h"
#include "../../IPassword.h"

#include "../FileManager/StringUtils.h"

#include "../Common/ExitCode.h"
#include "../Common/ArchiveCommandLine.h"

#include "ExtractRes.h"
#include "../Explorer/MyMessages.h"

#include "ExtractGUI.h"
#include "UpdateGUI.h"
#include "BenchmarkDialog.h"

using namespace NWindows;

HINSTANCE g_hInstance;
#ifndef _UNICODE
bool g_IsNT = false;
#endif

// static const wchar_t *kExceptionErrorMessage = L"Error:";
// static const wchar_t *kUserBreak  = L"Break signaled";

static const wchar_t *kMemoryExceptionMessage = L"ERROR: Can't allocate required memory!";
static const wchar_t *kUnknownExceptionMessage = L"Unknown Error";
// static const wchar_t *kInternalExceptionMessage = L"Internal Error #";
// static const wchar_t *kIncorrectCommandMessage = L"Incorrect command";

static void ErrorMessage(const wchar_t *message)
{
  MessageBoxW(0, message, L"7-Zip GUI", MB_ICONERROR);
}

int Main2()
{
  /*
  TCHAR t[512];
  GetCurrentDirectory(512, t);
  ErrorMessage(t);
  return 0;
  */

  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
  if(commandStrings.Size() <= 1)
  {
    MessageBoxW(0, L"Specify command", L"7-Zip", 0);
    return 0;
  }
  commandStrings.Delete(0);

  CArchiveCommandLineOptions options;
  CArchiveCommandLineParser parser;

  parser.Parse1(commandStrings, options);
  parser.Parse2(options);

  #ifdef _WIN32
  if (options.LargePages)
    NSecurity::EnableLockMemoryPrivilege();
  #endif

  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
    throw CSystemException(result);
  
  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();
 
  if (options.Command.CommandType == NCommandType::kBenchmark)
  {
    HRESULT res = Benchmark(
      #ifdef EXTERNAL_LZMA
      codecs,
      #endif
      options.NumThreads, options.DictionarySize);
    if (res != S_OK)
      throw CSystemException(res);
  }
  else if (isExtractGroupCommand)
  {
    CExtractCallbackImp *ecs = new CExtractCallbackImp;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
    ecs->ProgressDialog.CompressingMode = false;
    ecs->PasswordIsDefined = options.PasswordEnabled;
    ecs->Password = options.Password;
    ecs->Init();

    COpenCallbackGUI openCallback;
    openCallback.PasswordIsDefined = options.PasswordEnabled;
    openCallback.Password = options.Password;

    CExtractOptions eo;
    eo.StdOutMode = options.StdOutMode;
    eo.OutputDir = options.OutputDir;
    eo.YesToAll = options.YesToAll;
    eo.OverwriteMode = options.OverwriteMode;
    eo.PathMode = options.Command.GetPathMode();
    eo.TestMode = options.Command.IsTestMode();
    #ifdef COMPRESS_MT
    eo.Properties = options.ExtractProperties;
    #endif

    HRESULT result = ExtractGUI(codecs,
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          eo, options.ShowDialog, &openCallback, ecs);
    if (result != S_OK)
      throw CSystemException(result);
    if (ecs->Messages.Size() > 0 || ecs->NumArchiveErrors != 0)
      return NExitCode::kFatalError;    
  }
  else if (options.Command.IsFromUpdateGroup())
  {
    bool passwordIsDefined = 
        options.PasswordEnabled && !options.Password.IsEmpty();

    COpenCallbackGUI openCallback;
    openCallback.PasswordIsDefined = passwordIsDefined;
    openCallback.Password = options.Password;

    CUpdateCallbackGUI callback;
    // callback.EnablePercents = options.EnablePercents;
    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;
    // callback.StdOutMode = options.UpdateOptions.StdOutMode;
    callback.Init();

    CUpdateErrorInfo errorInfo;

    if (!options.UpdateOptions.Init(codecs, options.ArchiveName, options.ArcType))
      throw "Unsupported archive type";
    HRESULT result = UpdateGUI(
        codecs,
        options.WildcardCensor, options.UpdateOptions, 
        options.ShowDialog,
        errorInfo, &openCallback, &callback);

    if (result != S_OK)
    {
      if (!errorInfo.Message.IsEmpty())
        ErrorMessage(errorInfo.Message);
      throw CSystemException(result);
    }
    if (callback.FailedFiles.Size() > 0)
      return NExitCode::kWarning;    
  }
  else
  {
    ErrorMessage(L"Use correct command");
    return 0;
  }
  return 0;
}

static bool inline IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* lpCmdLine */, int /* nCmdShow */)
{
  g_hInstance = hInstance;
  #ifdef _UNICODE
  if (!IsItWindowsNT())
  {
    MyMessageBox(L"This program requires Windows NT/2000/XP/2003");
    return NExitCode::kFatalError;
  }
  #else
  g_IsNT = IsItWindowsNT();
  #endif

  #ifdef _WIN32
  SetLargePageSize();
  #endif

  InitCommonControls();

  ReloadLang();

  // setlocale(LC_COLLATE, ".ACP");
  try
  {
    return Main2();
  }
  catch(const CNewException &)
  {
    MyMessageBox(kMemoryExceptionMessage);
    return (NExitCode::kMemoryError);
  }
  catch(const CArchiveCommandLineException &e)
  {
    MyMessageBox(GetUnicodeString(e));
    return (NExitCode::kUserError);
  }
  catch(const CSystemException &systemError)
  {
    if (systemError.ErrorCode == E_OUTOFMEMORY)
    {
      MyMessageBox(kMemoryExceptionMessage);
      return (NExitCode::kMemoryError);
    }
    if (systemError.ErrorCode == E_ABORT)
    {
      // MyMessageBox(kUserBreak);
      return (NExitCode::kUserBreak);
    }
    UString message;
    NError::MyFormatMessage(systemError.ErrorCode, message);
    MyMessageBox(message);
    return (NExitCode::kFatalError);
  }
  /*
  catch(NExitCode::EEnum &exitCode)
  {
    g_StdErr << kInternalExceptionMessage << exitCode << endl;
    return (exitCode);
  }
  */
  catch(const UString &s)
  {
    MyMessageBox(s);
    return (NExitCode::kFatalError);
  }
  catch(const AString &s)
  {
    MyMessageBox(GetUnicodeString(s));
    return (NExitCode::kFatalError);
  }
  catch(const char *s)
  {
    MyMessageBox(GetUnicodeString(s));
    return (NExitCode::kFatalError);
  }
  /*
  catch(int t)
  {
    g_StdErr << kInternalExceptionMessage << t << endl;
    return (NExitCode::kFatalError);
  }
  */
  catch(...)
  {
    MyMessageBox(kUnknownExceptionMessage);
    return (NExitCode::kFatalError);
  }
}

