// ArchiveCommandLine.h

#ifndef __ARCHIVECOMMANDLINE_H
#define __ARCHIVECOMMANDLINE_H

#include "Common/Wildcard.h"

#include "Extract.h"
#include "Update.h"

namespace NCommandType { enum EEnum
{
  kAdd = 0,
  kUpdate,
  kDelete,
  kTest,
  kExtract,
  kFullExtract,
  kList
};}

namespace NRecursedType { enum EEnum
{
  kRecursed,
  kWildCardOnlyRecursed,
  kNonRecursed,
};}

struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
  NRecursedType::EEnum DefaultRecursedType() const;
  bool IsFromExtractGroup() const;
  bool IsFromUpdateGroup() const;
  bool IsTestMode() const { return CommandType == NCommandType::kTest; }
  NExtract::NPathMode::EEnum GetPathMode() const;
};

struct CArchiveCommandLineOptions
{
  bool HelpMode;

  bool IsInTerminal;
  bool IsOutTerminal;
  bool StdInMode;
  bool StdOutMode;
  bool EnableHeaders;
  bool YesToAll;
  bool ShowDialog;
  // NWildcard::CCensor ArchiveWildcardCensor;
  NWildcard::CCensor WildcardCensor;

  CArchiveCommand Command; 
  UString ArchiveName;

  bool PasswordEnabled;
  UString Password;

  // Extract
  bool AppendName;
  UString OutputDir;
  NExtract::NOverwriteMode::EEnum OverwriteMode;
  UStringVector ArchivePathsSorted;
  UStringVector ArchivePathsFullSorted;

  CUpdateOptions UpdateOptions;
  bool EnablePercents;

  // UString ArchiveType;
  // bool SfxMode;
};

int ParseCommandLine(UStringVector commandStrings,
    CArchiveCommandLineOptions &options);

#endif



