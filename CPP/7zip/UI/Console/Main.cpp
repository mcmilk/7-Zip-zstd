// Main.cpp

#include "StdAfx.h"

#include <io.h>

#include "Common/MyInitGuid.h"
#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"
#include "Common/StdInStream.h"
#include "Common/StringToInt.h"
#include "Common/Exception.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"
#include "Windows/Error.h"
// #include "Windows/System.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#endif

#include "../../IPassword.h"
#include "../../ICoder.h"
#include "../Common/ArchiverInfo.h"
#include "../Common/UpdateAction.h"
#include "../Common/Update.h"
#include "../Common/Extract.h"
#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"

#include "List.h"
#include "OpenCallbackConsole.h"
#include "ExtractCallbackConsole.h"
#include "UpdateCallbackConsole.h"

#include "../../MyVersion.h"

#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

HINSTANCE g_hInstance = 0;
extern CStdOutStream *g_StdStream;

static const char *kCopyrightString = "\n7-Zip"
#ifdef EXCLUDE_COM
" (A)"
#endif

#ifdef UNICODE
" [NT]"
#endif

" " MY_VERSION_COPYRIGHT_DATE "\n";

static const char *kHelpString = 
    "\nUsage: 7z"
#ifdef _NO_CRYPTO
    "r"
#elif EXCLUDE_COM
    "a"
#endif
    " <command> [<switches>...] <archive_name> [<file_names>...]\n"
    "       [<@listfiles...>]\n"
    "\n"
    "<Commands>\n"
    "  a: Add files to archive\n"
    "  d: Delete files from archive\n"
    "  e: Extract files from archive (without using directory names)\n"
    "  l: List contents of archive\n"
//    "  l[a|t][f]: List contents of archive\n"
//    "    a - with Additional fields\n"
//    "    t - with all fields\n"
//    "    f - with Full pathnames\n"
    "  t: Test integrity of archive\n"
    "  u: Update files to archive\n"
    "  x: eXtract files with full paths\n"
    "<Switches>\n"
    "  -ai[r[-|0]]{@listfile|!wildcard}: Include archives\n"
    "  -ax[r[-|0]]{@listfile|!wildcard}: eXclude archives\n"
    "  -bd: Disable percentage indicator\n"
    "  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
    "  -m{Parameters}: set compression Method\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -r[-|0]: Recurse subdirectories\n"
    "  -scs{UTF-8 | WIN | DOS}: set charset for list files\n"
    "  -sfx[{name}]: Create SFX archive\n"
    "  -si[{name}]: read data from stdin\n"
    "  -slt: show technical information for l (List) command\n"
    "  -so: write data to stdout\n"
    "  -t{Type}: Set type of archive\n"
    "  -v{Size}[b|k|m|g]: Create volumes\n"
    "  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName]: Update options\n"
    "  -w[{path}]: assign Work directory. Empty path means a temporary directory\n"
    "  -x[r[-|0]]]{@listfile|!wildcard}: eXclude filenames\n"
    "  -y: assume Yes on all queries\n";

// ---------------------------
// exception messages

static const char *kProcessArchiveMessage = " archive: ";
static const char *kEverythingIsOk = "Everything is Ok";
static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError

static const wchar_t *kDefaultSfxModule = L"7zCon.sfx";

static void ShowMessageAndThrowException(CStdOutStream &s, LPCSTR message, NExitCode::EEnum code)
{
  s << message << endl;
  throw code;
}

static void PrintHelpAndExit(CStdOutStream &s) // yyy
{
  s << kHelpString;
  ShowMessageAndThrowException(s, kUserErrorMessage, NExitCode::kUserError);
}

#ifndef _WIN32
static void GetArguments(int numArguments, const char *arguments[], UStringVector &parts)
{
  parts.Clear();
  for(int i = 0; i < numArguments; i++)
  {
    UString s = MultiByteToUnicodeString(arguments[i]);
    parts.Add(s);
  }
}
#endif

static void ShowCopyrightAndHelp(CStdOutStream &s, bool needHelp)
{
  s << kCopyrightString;
  /*
  UInt32 numCPUs = NWindows::NSystem::GetNumberOfProcessors();
  s << "System configuration: " << (UInt64)numCPUs << " CPU";
  if (numCPUs > 1) 
    s << "s";
  s << "\n";
  */
  if (needHelp) 
    s << kHelpString;
}

int Main2(
  #ifndef _WIN32  
  int numArguments, const char *arguments[]
  #endif
)
{
  #ifdef _WIN32  
  SetFileApisToOEM();
  #endif
  
  UStringVector commandStrings;
  #ifdef _WIN32  
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);
  #else
  GetArguments(numArguments, arguments, commandStrings);
  #endif

  if(commandStrings.Size() == 1)
  {
    ShowCopyrightAndHelp(g_StdOut, true);
    return 0;
  }
  commandStrings.Delete(0);

  CArchiveCommandLineOptions options;

  CArchiveCommandLineParser parser;

  parser.Parse1(commandStrings, options);

  if(options.HelpMode)
  {
    ShowCopyrightAndHelp(g_StdOut, true);
    return 0;
  }

  #ifdef _WIN32
  if (options.LargePages)
    NSecurity::EnableLockMemoryPrivilege();
  #endif

  CStdOutStream &stdStream = options.StdOutMode ? g_StdErr : g_StdOut;
  g_StdStream = &stdStream;

  if (options.EnableHeaders)
    ShowCopyrightAndHelp(stdStream, false);

  parser.Parse2(options);

  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();
  if(isExtractGroupCommand || 
      options.Command.CommandType == NCommandType::kList)
  {
    if(isExtractGroupCommand)
    {
      CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
      CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

      ecs->OutStream = &stdStream;
      ecs->PasswordIsDefined = options.PasswordEnabled;
      ecs->Password = options.Password;
      ecs->Init();

      COpenCallbackConsole openCallback;
      openCallback.OutStream = &stdStream;
      openCallback.PasswordIsDefined = options.PasswordEnabled;
      openCallback.Password = options.Password;

      CExtractOptions eo;
      eo.StdOutMode = options.StdOutMode;
      eo.PathMode = options.Command.GetPathMode();
      eo.TestMode = options.Command.IsTestMode();
      eo.OverwriteMode = options.OverwriteMode;
      eo.OutputDir = options.OutputDir;
      eo.YesToAll = options.YesToAll;
      #ifdef COMPRESS_MT
      eo.Properties = options.ExtractProperties;
      #endif
      UString errorMessage;
      HRESULT result = DecompressArchives(
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          eo, &openCallback, ecs, errorMessage);
      if (!errorMessage.IsEmpty())
      {
        stdStream << endl << "Error: " << errorMessage;
        if (result == S_OK)
          result = E_FAIL;
      }

      if (ecs->NumArchives > 1)
      {
        stdStream << endl << endl << "Total:" << endl;
        stdStream << "Archives: " << ecs->NumArchives << endl;
      }
      if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
      {
        if (ecs->NumArchives > 1)
        {
          if (ecs->NumArchiveErrors != 0)
            stdStream << "Archive Errors: " << ecs->NumArchiveErrors << endl;
          if (ecs->NumFileErrors != 0)
            stdStream << "Sub items Errors: " << ecs->NumFileErrors << endl;
        }
        if (result != S_OK)
          throw CSystemException(result);
        return NExitCode::kFatalError;
      }
      if (result != S_OK)
        throw CSystemException(result);
    }
    else
    {
      HRESULT result = ListArchives(
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          options.EnableHeaders, 
          options.TechMode,
          options.PasswordEnabled, 
          options.Password);
      if (result != S_OK)
        throw CSystemException(result);
    }
  }
  else if(options.Command.IsFromUpdateGroup())
  {
    UString workingDir;

    CUpdateOptions &uo = options.UpdateOptions;
    if (uo.SfxMode && uo.SfxModule.IsEmpty())
      uo.SfxModule = kDefaultSfxModule;

    bool passwordIsDefined = 
        options.PasswordEnabled && !options.Password.IsEmpty();

    COpenCallbackConsole openCallback;
    openCallback.OutStream = &stdStream;
    openCallback.PasswordIsDefined = passwordIsDefined;
    openCallback.Password = options.Password;

    CUpdateCallbackConsole callback;
    callback.EnablePercents = options.EnablePercents;
    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;
    callback.StdOutMode = uo.StdOutMode;
    callback.Init(&stdStream);

    CUpdateErrorInfo errorInfo;

    HRESULT result = UpdateArchive(options.WildcardCensor, uo, 
        errorInfo, &openCallback, &callback);

    int exitCode = NExitCode::kSuccess;
    if (callback.CantFindFiles.Size() > 0)
    {
      stdStream << endl;
      stdStream << "WARNINGS for files:" << endl << endl;
      int numErrors = callback.CantFindFiles.Size();
      for (int i = 0; i < numErrors; i++)
      {
        stdStream << callback.CantFindFiles[i] << " : ";
        stdStream << NError::MyFormatMessageW(callback.CantFindCodes[i]) << endl;
      }
      stdStream << "----------------" << endl;
      stdStream << "WARNING: Cannot find " << numErrors << " file";
      if (numErrors > 1)
        stdStream << "s";
      stdStream << endl;
      exitCode = NExitCode::kWarning;
    }

    if (result != S_OK)
    {
      UString message;
      if (!errorInfo.Message.IsEmpty())
      {
        message += errorInfo.Message;
        message += L"\n";
      }
      if (!errorInfo.FileName.IsEmpty())
      {
        message += errorInfo.FileName;
        message += L"\n";
      }
      if (!errorInfo.FileName2.IsEmpty())
      {
        message += errorInfo.FileName2;
        message += L"\n";
      }
      if (errorInfo.SystemError != 0)
      {
        message += NError::MyFormatMessageW(errorInfo.SystemError);
        message += L"\n";
      }
      if (!message.IsEmpty())
        stdStream << L"\nError:\n" << message;
      throw CSystemException(result);
    }
    int numErrors = callback.FailedFiles.Size();
    if (numErrors == 0)
    {
      if (callback.CantFindFiles.Size() == 0)
        stdStream << kEverythingIsOk << endl;
    }
    else
    {
      stdStream << endl;
      stdStream << "WARNINGS for files:" << endl << endl;
      for (int i = 0; i < numErrors; i++)
      {
        stdStream << callback.FailedFiles[i] << " : ";
        stdStream << NError::MyFormatMessageW(callback.FailedCodes[i]) << endl;
      }
      stdStream << "----------------" << endl;
      stdStream << "WARNING: Cannot open " << numErrors << " file";
      if (numErrors > 1)
        stdStream << "s";
      stdStream << endl;
      exitCode = NExitCode::kWarning;
    }
    return exitCode;
  }
  else 
    PrintHelpAndExit(stdStream);
  return 0;
}
