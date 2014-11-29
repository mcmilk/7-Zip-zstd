// ArchiveCommandLine.h

#ifndef __ARCHIVE_COMMAND_LINE_H
#define __ARCHIVE_COMMAND_LINE_H

#include "../../../Common/CommandLineParser.h"
#include "../../../Common/Wildcard.h"

#include "Extract.h"
#include "HashCalc.h"
#include "Update.h"

struct CArcCmdLineException: public UString
{
  CArcCmdLineException(const char *a, const wchar_t *u = NULL);
};

namespace NCommandType { enum EEnum
{
  kAdd = 0,
  kUpdate,
  kDelete,
  kTest,
  kExtract,
  kExtractFull,
  kList,
  kBenchmark,
  kInfo,
  kHash,
  kRename
};}

struct CArcCommand
{
  NCommandType::EEnum CommandType;

  bool IsFromExtractGroup() const;
  bool IsFromUpdateGroup() const;
  bool IsTestCommand() const { return CommandType == NCommandType::kTest; }
  NExtract::NPathMode::EEnum GetPathMode() const;
};

struct CArcCmdLineOptions
{
  bool HelpMode;

  #ifdef _WIN32
  bool LargePages;
  #endif
  bool CaseSensitiveChange;
  bool CaseSensitive;

  bool IsInTerminal;
  bool IsStdOutTerminal;
  bool IsStdErrTerminal;
  bool StdInMode;
  bool StdOutMode;
  bool EnableHeaders;

  bool YesToAll;
  bool ShowDialog;
  NWildcard::CCensor Censor;

  CArcCommand Command;
  UString ArchiveName;

  #ifndef _NO_CRYPTO
  bool PasswordEnabled;
  UString Password;
  #endif

  bool TechMode;
  
  UStringVector HashMethods;

  bool AppendName;
  UStringVector ArchivePathsSorted;
  UStringVector ArchivePathsFullSorted;
  CObjectVector<CProperty> Properties;

  CExtractOptionsBase ExtractOptions;

  CBoolPair NtSecurity;
  CBoolPair AltStreams;
  CBoolPair HardLinks;
  CBoolPair SymLinks;

  CUpdateOptions UpdateOptions;
  CHashOptions HashOptions;
  UString ArcType;
  UStringVector ExcludedArcTypes;
  bool EnablePercents;

  // Benchmark
  UInt32 NumIterations;

  CArcCmdLineOptions():
      StdInMode(false),
      StdOutMode(false),
      CaseSensitiveChange(false),
      CaseSensitive(false)
      {};
};

class CArcCmdLineParser
{
  NCommandLineParser::CParser parser;
public:
  CArcCmdLineParser();
  void Parse1(const UStringVector &commandStrings, CArcCmdLineOptions &options);
  void Parse2(CArcCmdLineOptions &options);
};

void EnumerateDirItemsAndSort(
    bool storeAltStreams,
    NWildcard::CCensor &censor,
    NWildcard::ECensorPathMode pathMode,
    const UString &addPathPrefix,
    UStringVector &sortedPaths,
    UStringVector &sortedFullPaths);

#endif
