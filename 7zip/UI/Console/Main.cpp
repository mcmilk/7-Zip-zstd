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

#include "../../IPassword.h"
#include "../../ICoder.h"
#include "../../Compress/LZ/IMatchFinder.h"
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

" 4.24 beta  Copyright (c) 1999-2005 Igor Pavlov  2005-07-06\n";

static const char *kHelpString = 
    "\nUsage: 7z"
#ifdef EXCLUDE_COM
    "a"
#endif
    " <command> [<switches>...] <archive_name> [<file_names>...]\n"
    "       [<@listfiles...>]\n"
    "\n"
    "<Commands>\n"
    "  a: Add files to archive\n"
    "  d: Delete files from archive\n"
    "  e: Extract files from archive\n"
    "  l: List contents of archive\n"
//    "  l[a|t][f]: List contents of archive\n"
//    "    a - with Additional fields\n"
//    "    t - with all fields\n"
//    "    f - with Full pathnames\n"
    "  t: Test integrity of archive\n"
    "  u: Update files to archive\n"
    "  x: eXtract files with full pathname\n"
    "<Switches>\n"
    "  -ai[r[-|0]]{@listfile|!wildcard}: Include archives\n"
    "  -ax[r[-|0]]{@listfile|!wildcard}: eXclude archives\n"
    "  -bd: Disable percentage indicator\n"
    "  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
    "  -m{Parameters}: set compression Method\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -r[-|0]: Recurse subdirectories\n"
    "  -sfx[{name}]: Create SFX archive\n"
    "  -si: read data from stdin\n"
    "  -so: write data to stdout\n"
    "  -t{Type}: Set type of archive\n"
    "  -v{Size}}[b|k|m|g]: Create volumes\n"
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

static void PrintHelp(CStdOutStream &s)
{
  s << kHelpString;
}

static void ShowMessageAndThrowException(CStdOutStream &s, LPCSTR message, NExitCode::EEnum code)
{
  s << message << endl;
  throw code;
}

static void PrintHelpAndExit(CStdOutStream &s) // yyy
{
  PrintHelp(s);
  ShowMessageAndThrowException(s, kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(CStdOutStream &s, const AString &processTitle, const UString &archiveName)
{
  s << endl << processTitle << kProcessArchiveMessage << archiveName << endl << endl;
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
    g_StdOut << kCopyrightString;
    g_StdOut << kHelpString;
    return 0;
  }
  commandStrings.Delete(0);

  CArchiveCommandLineOptions options;

  CArchiveCommandLineParser parser;

  parser.Parse1(commandStrings, options);

  if(options.HelpMode)
  {
    g_StdOut << kCopyrightString;
    PrintHelp(g_StdOut);
    return 0;
  }

  CStdOutStream &stdStream = options.StdOutMode ? g_StdErr : g_StdOut;
  g_StdStream = &stdStream;

  if (options.EnableHeaders)
    stdStream << kCopyrightString;

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
      HRESULT result = DecompressArchives(
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          eo, &openCallback, ecs);

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

    HRESULT result = UpdateArchive(
        options.WildcardCensor, uo, 
        errorInfo, &openCallback, &callback);

    if (result != S_OK)
    {
      stdStream << "\nError:\n";
      if (!errorInfo.Message.IsEmpty())
        stdStream << errorInfo.Message << endl;
      if (!errorInfo.FileName.IsEmpty())
        stdStream << errorInfo.FileName << endl;
      if (!errorInfo.FileName2.IsEmpty())
        stdStream << errorInfo.FileName2 << endl;
      if (errorInfo.SystemError != 0)
        stdStream << NError::MyFormatMessageW(errorInfo.SystemError) << endl;
      throw CSystemException(result);
    }
    int exitCode = NExitCode::kSuccess;
    int numErrors = callback.FailedFiles.Size();
    if (numErrors == 0)
      stdStream << kEverythingIsOk << endl;
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
