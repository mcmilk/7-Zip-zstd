// GUI.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"

#include "../../../../C/Alloc.h"

#include "Common/CommandLineParser.h"
#include "Common/MyException.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/NtCheck.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#endif

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"

#include "../FileManager/StringUtils.h"

#include "BenchmarkDialog.h"
#include "ExtractGUI.h"
#include "UpdateGUI.h"

#include "ExtractRes.h"

using namespace NWindows;

HINSTANCE g_hInstance;
#ifndef _UNICODE
#endif

#ifdef UNDER_CE
bool g_LVN_ITEMACTIVATE_Support = true;
#endif

static void ErrorMessage(LPCWSTR message)
{
  MessageBoxW(NULL, message, L"7-Zip", MB_ICONERROR | MB_OK);
}

static void ErrorLangMessage(UINT resourceID, UInt32 langID)
{
  ErrorMessage(LangString(resourceID, langID));
}

static const char *kNoFormats = "7-Zip cannot find the code that works with archives.";

static int ShowMemErrorMessage()
{
  ErrorLangMessage(IDS_MEM_ERROR, 0x0200060B);
  return NExitCode::kMemoryError;
}

static int ShowSysErrorMessage(DWORD errorCode)
{
  if (errorCode == E_OUTOFMEMORY)
    return ShowMemErrorMessage();
  ErrorMessage(HResultToMessage(errorCode));
  return NExitCode::kFatalError;
}

static int Main2()
{
  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);

  #ifndef UNDER_CE
  if (commandStrings.Size() > 0)
    commandStrings.Delete(0);
  #endif
  if (commandStrings.Size() == 0)
  {
    MessageBoxW(0, L"Specify command", L"7-Zip", 0);
    return 0;
  }

  CArchiveCommandLineOptions options;
  CArchiveCommandLineParser parser;

  parser.Parse1(commandStrings, options);
  parser.Parse2(options);

  #if defined(_WIN32) && defined(_7ZIP_LARGE_PAGES)
  if (options.LargePages)
    NSecurity::EnableLockMemoryPrivilege();
  #endif

  CCodecs *codecs = new CCodecs;
  CMyComPtr<IUnknown> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
    throw CSystemException(result);
  
  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();
  if (codecs->Formats.Size() == 0 &&
        (isExtractGroupCommand ||
        options.Command.IsFromUpdateGroup()))
    throw kNoFormats;

  CIntVector formatIndices;
  if (!codecs->FindFormatForArchiveType(options.ArcType, formatIndices))
  {
    ErrorLangMessage(IDS_UNSUPPORTED_ARCHIVE_TYPE, 0x0200060D);
    return NExitCode::kFatalError;
  }
 
  if (options.Command.CommandType == NCommandType::kBenchmark)
  {
    HRESULT res;
    #ifdef EXTERNAL_CODECS
    CObjectVector<CCodecInfoEx> externalCodecs;
    res = LoadExternalCodecs(codecs, externalCodecs);
    if (res != S_OK)
      throw CSystemException(res);
    #endif
    res = Benchmark(
      #ifdef EXTERNAL_CODECS
      codecs, &externalCodecs,
      #endif
      options.NumThreads, options.DictionarySize);
    if (res != S_OK)
      throw CSystemException(res);
  }
  else if (isExtractGroupCommand)
  {
    CExtractCallbackImp *ecs = new CExtractCallbackImp;
    CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

    #ifndef _NO_CRYPTO
    ecs->PasswordIsDefined = options.PasswordEnabled;
    ecs->Password = options.Password;
    #endif

    ecs->Init();

    CExtractOptions eo;
    eo.StdOutMode = options.StdOutMode;
    eo.OutputDir = options.OutputDir;
    eo.YesToAll = options.YesToAll;
    eo.OverwriteMode = options.OverwriteMode;
    eo.PathMode = options.Command.GetPathMode();
    eo.TestMode = options.Command.IsTestMode();
    eo.CalcCrc = options.CalcCrc;
    #if !defined(_7ZIP_ST) && !defined(_SFX)
    eo.Properties = options.ExtractProperties;
    #endif

    bool messageWasDisplayed = false;
    HRESULT result = ExtractGUI(codecs, formatIndices,
          options.ArchivePathsSorted,
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head,
          eo, options.ShowDialog, messageWasDisplayed, ecs);
    if (result != S_OK)
    {
      if (result != E_ABORT && messageWasDisplayed)
        return NExitCode::kFatalError;
      throw CSystemException(result);
    }
    if (!ecs->IsOK())
      return NExitCode::kFatalError;
  }
  else if (options.Command.IsFromUpdateGroup())
  {
    #ifndef _NO_CRYPTO
    bool passwordIsDefined = options.PasswordEnabled && !options.Password.IsEmpty();
    #endif

    CUpdateCallbackGUI callback;
    // callback.EnablePercents = options.EnablePercents;

    #ifndef _NO_CRYPTO
    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;
    #endif

    // callback.StdOutMode = options.UpdateOptions.StdOutMode;
    callback.Init();

    if (!options.UpdateOptions.Init(codecs, formatIndices, options.ArchiveName))
    {
      ErrorLangMessage(IDS_UPDATE_NOT_SUPPORTED, 0x02000601);
      return NExitCode::kFatalError;
    }
    bool messageWasDisplayed = false;
    HRESULT result = UpdateGUI(
        codecs,
        options.WildcardCensor, options.UpdateOptions,
        options.ShowDialog,
        messageWasDisplayed, &callback);

    if (result != S_OK)
    {
      if (result != E_ABORT && messageWasDisplayed)
        return NExitCode::kFatalError;
      throw CSystemException(result);
    }
    if (callback.FailedFiles.Size() > 0)
    {
      if (!messageWasDisplayed)
        throw CSystemException(E_FAIL);
      return NExitCode::kWarning;
    }
  }
  else
  {
    throw "Unsupported command";
  }
  return 0;
}

#define NT_CHECK_FAIL_ACTION ErrorMessage(L"Unsupported Windows version"); return NExitCode::kFatalError;

int APIENTRY WinMain(HINSTANCE  hInstance, HINSTANCE /* hPrevInstance */,
  #ifdef UNDER_CE
  LPWSTR
  #else
  LPSTR
  #endif
  /* lpCmdLine */, int /* nCmdShow */)
{
  g_hInstance = hInstance;
  #ifdef _WIN32
  NT_CHECK
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
    return ShowMemErrorMessage();
  }
  catch(const CArchiveCommandLineException &e)
  {
    ErrorMessage(GetUnicodeString(e));
    return NExitCode::kUserError;
  }
  catch(const CSystemException &systemError)
  {
    if (systemError.ErrorCode == E_ABORT)
      return NExitCode::kUserBreak;
    return ShowSysErrorMessage(systemError.ErrorCode);
  }
  catch(const UString &s)
  {
    ErrorMessage(s);
    return NExitCode::kFatalError;
  }
  catch(const AString &s)
  {
    ErrorMessage(GetUnicodeString(s));
    return NExitCode::kFatalError;
  }
  catch(const wchar_t *s)
  {
    ErrorMessage(s);
    return NExitCode::kFatalError;
  }
  catch(const char *s)
  {
    ErrorMessage(GetUnicodeString(s));
    return NExitCode::kFatalError;
  }
  catch(...)
  {
    ErrorLangMessage(IDS_UNKNOWN_ERROR, 0x0200060C);
    return NExitCode::kFatalError;
  }
}

