// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/StringConvert.h"
#include "Common/MyCom.h"
#include "Common/Exception.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"

#include "../../IPassword.h"
#include "../../ICoder.h"

#include "../../UI/Common/OpenArchive.h"
#include "../../UI/Common/ZipRegistry.h"
#include "../../UI/Common/DefaultName.h"
#include "../../UI/Common/ExitCode.h"
#include "../../UI/Common/Extract.h"

// #include "../../UI/Console/Extract.h"
// #include "../../UI/Console/ArError.h"
#include "../../UI/Console/List.h"
#include "../../UI/Console/OpenCallbackConsole.h"
#include "../../UI/Console/ExtractCallbackConsole.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

static const char *kCopyrightString = 
"\n7-Zip SFX 4.10 beta  Copyright (c) 1999-2004 Igor Pavlov  2004-10-21\n";

static const int kNumSwitches = 6;

const wchar_t *defaultExt = L".exe";

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDisablePercents,
  kYes,
  kPassword,
  kOutputDir
};

}

namespace NRecursedType {
enum EEnum
{
  kRecursed,
  kWildCardOnlyRecursed,
  kNonRecursed,
};
}

static const char kRecursedIDChar = 'R';
static const wchar_t *kRecursedPostCharSet = L"0-";

namespace NRecursedPostCharIndex {
  enum EEnum 
  {
    kWildCardRecursionOnly = 0, 
    kNoRecursion = 1
  };
}

static const char kFileListID = '@';
static const char kImmediateNameID = '!';

static const char kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const char kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be

static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"Y",  NSwitchType::kSimple, false },
    { L"P",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
  };

static const int kNumCommandForms = 3;

namespace NCommandType {
enum EEnum
{
  kTest = 0,
  // kExtract,
  kFullExtract,
  kList
};

}

static const CCommandForm commandForms[kNumCommandForms] = 
{
  { L"T", false },
  // { "E", false },
  { L"X", false },
  { L"L", false }
};

static const NRecursedType::EEnum kCommandRecursedDefault[kNumCommandForms] = 
{
  NRecursedType::kRecursed
};

static const bool kTestExtractRecursedDefault = true;
static const bool kAddRecursedDefault = false;

static const int kMaxCmdLineSize = 1000;
static const wchar_t *kUniversalWildcard = L"*";
static const int kMinNonSwitchWords = 1;
static const int kCommandIndex = 0;

static const char *kHelpString = 
    "\nUsage: 7zSFX [<command>] [<switches>...]\n"
    "\n"
    "<Commands>\n"
    "  l: List contents of archive\n"
    "  t: Test integrity of archive\n"
    "  x: eXtract files with full pathname (default)\n"
    "<Switches>\n"
    // "  -bd Disable percentage indicator\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -y: assume Yes on all queries\n";


// ---------------------------
// exception messages

static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError
static const char *kIncorrectListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInCommandLine  = "Incorrect wildcard in command line";

static const CSysString kFileIsNotArchiveMessageBefore = "File \"";
static const CSysString kFileIsNotArchiveMessageAfter = "\" is not archive";

static const char *kProcessArchiveMessage = " archive: ";


// ---------------------------

static const CSysString kExtractGroupProcessMessage = "Processing";
static const CSysString kListingProcessMessage = "Listing";

static const CSysString kDefaultWorkingDirectory = "";  // test it maybemust be "."

struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
  NRecursedType::EEnum DefaultRecursedType() const;
};

NRecursedType::EEnum CArchiveCommand::DefaultRecursedType() const
{
  return kCommandRecursedDefault[CommandType];
}

static NRecursedType::EEnum GetRecursedTypeFromIndex(int index)
{
  switch (index)
  {
    case NRecursedPostCharIndex::kWildCardRecursionOnly: 
      return NRecursedType::kWildCardOnlyRecursed;
    case NRecursedPostCharIndex::kNoRecursion: 
      return NRecursedType::kNonRecursed;
    default:
      return NRecursedType::kRecursed;
  }
}

void PrintHelp(void)
{
  g_StdOut << kHelpString;
}

static void ShowMessageAndThrowException(LPCTSTR message, NExitCode::EEnum code)
{
  g_StdOut << message << endl;
  throw code;
}

static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(const CSysString &processTitle, const UString &archiveName)
{
  g_StdOut << endl << processTitle << kProcessArchiveMessage << 
      archiveName << endl << endl;
}

bool ParseArchiveCommand(const UString &commandString, CArchiveCommand &command)
{
  UString commandStringUpper = commandString;
  commandStringUpper.MakeUpper();
  UString postString;
  int commandIndex = ParseCommand(kNumCommandForms, commandForms, commandStringUpper, 
      postString) ;
  if (commandIndex < 0)
    return false;
  command.CommandType = (NCommandType::EEnum)commandIndex;
  return true;
}

// ------------------------------------------------------------------
// filenames functions

static bool AddNameToCensor(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  /*
  if(!IsWildCardFilePathLegal(name))
    return false;
  */
  bool isWildCard = DoesNameContainWildCard(name);
  bool recursed;

  switch (type)
  {
    case NRecursedType::kWildCardOnlyRecursed:
      recursed = isWildCard;
      break;
    case NRecursedType::kRecursed:
      recursed = true;
      break;
    case NRecursedType::kNonRecursed:
      recursed = false;
      break;
  }
  wildcardCensor.AddItem(name, include, recursed);
  return true;
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!AddNameToCensor(wildcardCensor, name, include, type))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

static bool AreEqualNoCase(wchar_t c1, wchar_t c2)
{
  return ::MyCharUpper(c1) == ::MyCharUpper(c2);
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &wildcardCensor, 
    const UStringVector &nonSwitchStrings, NRecursedType::EEnum type, 
    bool thereAreSwitchIncludeWildCards)
{
  AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
}

/*
void AddSwitchWildCardsToCensor(NWildcard::CCensor &wildcardCensor, 
    const UStringVector &strings, bool include, NRecursedType::EEnum commonRecursedType)
{
  for(int i = 0; i < strings.Size(); i++)
  {
    const UString &string = strings[i];
    NRecursedType::EEnum recursedType;
    int pos = 0;
    if (string.Length() < kSomeCludePostStringMinSize)
      PrintHelpAndExit();
    if (AreEqualNoCase(string[pos], kRecursedIDChar))
    {
      pos++;
      int index = UString(kRecursedPostCharSet).Find(string[pos]);
      recursedType = GetRecursedTypeFromIndex(index);
      if (index >= 0)
        pos++;
    }
    else
      recursedType = commonRecursedType;
    if (string.Length() < pos + kSomeCludeAfterRecursedPostStringMinSize)
      PrintHelpAndExit();
    UString tail = string.Mid(pos + 1);
    if (AreEqualNoCase(string[pos], kImmediateNameID))
      AddCommandLineWildCardToCensr(wildcardCensor, 
          tail, include, recursedType);
    else 
      PrintHelpAndExit();
  }
}
*/

// ------------------------------------------------------------------
/*
static void ThrowPrintFileIsNotArchiveException(const CSysString &fileName)
{
  CSysString message;
  message = kFileIsNotArchiveMessageBefore + fileName + kFileIsNotArchiveMessageAfter;
  ShowMessageAndThrowException(message, NExitCode::kFileIsNotArchive);
}
*/

// int Main2(int numArguments, const char *arguments[])
int Main2()
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;
  
  UStringVector commandStrings;
  SplitCommandLine(GetCommandLineW(), commandStrings);

  UString archiveName = commandStrings.Front();

  commandStrings.Delete(0);

  NCommandLineParser::CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...) 
  {
    PrintHelpAndExit();
  }

  if(parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  int numNonSwitchStrings = nonSwitchStrings.Size();

  CArchiveCommand command;
  if (numNonSwitchStrings == 0)
    command.CommandType = NCommandType::kFullExtract;
  else 
  {
    if (numNonSwitchStrings > 1)
      PrintHelpAndExit();
    if (!ParseArchiveCommand(nonSwitchStrings[kCommandIndex], command))
      PrintHelpAndExit();
  }


  NRecursedType::EEnum recursedType;
  recursedType = command.DefaultRecursedType();

  NWildcard::CCensor wildcardCensor;
  
  bool thereAreSwitchIncludeWildCards;
  thereAreSwitchIncludeWildCards = false;
  AddToCensorFromNonSwitchesStrings(wildcardCensor, nonSwitchStrings, recursedType,
      thereAreSwitchIncludeWildCards);

  bool yesToAll = parser[NKey::kYes].ThereIs;

  if (archiveName.Right(4).CompareNoCase(defaultExt) != 0)
    archiveName += defaultExt;

  // NExtractMode::EEnum extractMode;
  // bool isExtractGroupCommand = command.IsFromExtractGroup(extractMode);

  bool passwordEnabled = parser[NKey::kPassword].ThereIs;

  UString password;
  if(passwordEnabled)
    password = parser[NKey::kPassword].PostStrings[0];

  NFind::CFileInfoW archiveFileInfo;
  if (!NFind::FindFile(archiveName, archiveFileInfo) || archiveFileInfo.IsDirectory())
    throw "there is no such archive";

  if (archiveFileInfo.IsDirectory())
    throw "there is no such archive";
  
  UString outputDir;
  if(parser[NKey::kOutputDir].ThereIs)
  {
    outputDir = parser[NKey::kOutputDir].PostStrings[0];
    NName::NormalizeDirPathPrefix(outputDir);
  }

  {
    UStringVector v1, v2;
    v1.Add(archiveName);
    v2.Add(archiveName);
    const NWildcard::CCensorNode &wildcardCensorHead = 
      wildcardCensor.Pairs.Front().Head;
    if(command.CommandType != NCommandType::kList)
    {
      CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
      CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
      ecs->PasswordIsDefined = passwordEnabled;
      ecs->Password = password;
      ecs->Init();

      COpenCallbackConsole openCallback;
      openCallback.PasswordIsDefined = passwordEnabled;
      openCallback.Password = password;

      CExtractOptions eo;
      eo.StdOutMode = false;
      eo.PathMode = NExtract::NPathMode::kFullPathnames;
      eo.TestMode = command.CommandType == NCommandType::kTest;
      eo.OverwriteMode = yesToAll ? 
          NExtract::NOverwriteMode::kWithoutPrompt : 
          NExtract::NOverwriteMode::kAskBefore;
      eo.OutputDir = outputDir;
      eo.YesToAll = yesToAll;

      HRESULT result = DecompressArchives(
          v1, v2,
          wildcardCensorHead, 
          eo, &openCallback, ecs);

      if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
      {
        if (ecs->NumArchiveErrors != 0)
          g_StdErr << endl << "Archive Errors: " << ecs->NumArchiveErrors << endl;
        if (ecs->NumFileErrors != 0)
          g_StdErr << endl << "Sub items Errors: " << ecs->NumFileErrors << endl;
        return NExitCode::kFatalError;
      }
      if (result != S_OK)
        throw CSystemException(result);
    }
    else
    {
      HRESULT result = ListArchives(
          v1, v2,
          wildcardCensorHead, 
          true, 
          passwordEnabled, 
          password);
      if (result != S_OK)
        throw CSystemException(result);
    }
  }
  return 0;
}
