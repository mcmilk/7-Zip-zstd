// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"

#include "../../IPassword.h"
#include "../../ICoder.h"

#include "../../UI/Common/OpenArchive.h"
#include "../../UI/Common/ZipRegistry.h"
#include "../../UI/Common/DefaultName.h"

#include "../../UI/Console/Extract.h"
#include "../../UI/Console/ArError.h"
#include "../../UI/Console/List.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

static const char *kCopyrightString = 
"\n7-Zip SFX 3.09.01 beta  Copyright (c) 1999-2003 Igor Pavlov  2003-09-06\n";

static const int kNumSwitches = 6;

const LPTSTR defaultExt = TEXT(".exe");

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
static const char *kRecursedPostCharSet = "0-";

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

static const kNumCommandForms = 3;

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

static const kMaxCmdLineSize = 1000;
static const wchar_t *kUniversalWildcard = L"*";
static const kMinNonSwitchWords = 1;
static const kCommandIndex = 0;

// static const kArchiveNameIndex = kCommandIndex + 1;
// static const kFirstFileNameIndex = kArchiveNameIndex + 1;

// static const kFirstFileNameIndex = kCommandIndex + 1;

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
    // "  -p{Password}: set Password\n"
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
  // NListMode::EEnum ListMode;
  // bool ListFullPathes;
  NRecursedType::EEnum DefaultRecursedType() const;
  bool IsFromExtractGroup(NExtractMode::EEnum &extractMode) const;
};

NRecursedType::EEnum CArchiveCommand::DefaultRecursedType() const
{
  return kCommandRecursedDefault[CommandType];
}

bool CArchiveCommand::IsFromExtractGroup(NExtractMode::EEnum &extractMode) const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
      extractMode = NExtractMode::kTest;
      return true;
    /*
    case NCommandType::kExtract:
      extractMode = NExtractMode::kExtractToOne;
      return true;
    */
    case NCommandType::kFullExtract:
      extractMode = NExtractMode::kFullPath;
      return true;
    default:
      return false;
  }
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

static void PrintProcessTitle(const CSysString &processTitle, const CSysString &archiveName)
{
  g_StdOut << endl << processTitle << kProcessArchiveMessage << archiveName << endl << endl;
}

void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    UStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
    strings.Add(MultiByteToUnicodeString(arguments[i]));
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
  wildcardCensor.AddItem(name, include, recursed, isWildCard);
  return true;
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!AddNameToCensor(wildcardCensor, name, include, type))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

static bool AreEqualNoCase(char c1, char c2)
{
  return ::MyCharUpper(c1) == ::MyCharUpper(c2);
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &wildcardCensor, 
    const UStringVector &nonSwitchStrings, NRecursedType::EEnum type, 
    bool thereAreSwitchIncludeWildCards)
{
  /*
  int numNonSwitchStrings = nonSwitchStrings.Size();
  if(numNonSwitchStrings == kMinNonSwitchWords && (!thereAreSwitchIncludeWildCards)) 
  */
    AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
  /*
  for(int i = kFirstFileNameIndex; i < numNonSwitchStrings; i++)
  {
    const AString &string = nonSwitchStrings[i];
    AddCommandLineWildCardToCensr(wildcardCensor, 
      MultiByteToUnicodeString(string, CP_OEMCP), true, type);
  }
  */
}

void AddSwitchWildCardsToCensor(NWildcard::CCensor &wildcardCensor, 
    const AStringVector &strings, bool include, NRecursedType::EEnum commonRecursedType)
{
  for(int i = 0; i < strings.Size(); i++)
  {
    const AString &string = strings[i];
    NRecursedType::EEnum recursedType;
    int pos = 0;
    if (string.Length() < kSomeCludePostStringMinSize)
      PrintHelpAndExit();
    if (AreEqualNoCase(string[pos], kRecursedIDChar))
    {
      pos++;
      int index = CSysString(kRecursedPostCharSet).Find(string[pos]);
      recursedType = GetRecursedTypeFromIndex(index);
      if (index >= 0)
        pos++;
    }
    else
      recursedType = commonRecursedType;
    if (string.Length() < pos + kSomeCludeAfterRecursedPostStringMinSize)
      PrintHelpAndExit();
    AString tail = string.Mid(pos + 1);
    if (AreEqualNoCase(string[pos], kImmediateNameID))
      AddCommandLineWildCardToCensr(wildcardCensor, 
          MultiByteToUnicodeString(tail, CP_OEMCP), include, recursedType);
    else 
      PrintHelpAndExit();
  }
}

// ------------------------------------------------------------------
static void ThrowPrintFileIsNotArchiveException(const CSysString &fileName)
{
  CSysString message;
  message = kFileIsNotArchiveMessageBefore + fileName + kFileIsNotArchiveMessageAfter;
  ShowMessageAndThrowException(message, NExitCode::kFileIsNotArchive);
}

void MyOpenArhive(const CSysString &archiveName, 
  const NFind::CFileInfo &archiveFileInfo,
  IInArchive **archiveHandler,
  UString &defaultItemName)
{
  CArchiverInfo archiverInfo;
  int subExtIndex;
  HRESULT result = OpenArchive(archiveName, archiveHandler, 
      archiverInfo, subExtIndex, 0);
  if (result == S_FALSE)
    throw "file is not supported archive";
  if (result != S_OK)
    throw "error";
  defaultItemName = GetDefaultName(archiveName, 
      archiverInfo.Extensions[subExtIndex].Extension, 
      archiverInfo.Extensions[subExtIndex].AddExtension);
}

int Main2(int numArguments, const char *arguments[])
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;
  
  /*
  if(numArguments == 1)
  {
    PrintHelp();
    return 0;
  }
  */
  UStringVector commandStrings;
  WriteArgumentsToStringList(numArguments, arguments, commandStrings);

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
  const UStringVector &nonSwitchStrings = parser._nonSwitchStrings;

  int numNonSwitchStrings = nonSwitchStrings.Size();
  /*
  if(numNonSwitchStrings < kMinNonSwitchWords)  
    PrintHelpAndExit();
  */
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

  CSysString archiveName;
  
  // archiveName = nonSwitchStrings[kArchiveNameIndex];
  archiveName = arguments[0];
  
  // g_StdOut << archiveName << endl;
  // g_StdOut << GetCommandLine() << endl;

  if (archiveName.Right(4).CompareNoCase(defaultExt) != 0)
    archiveName += defaultExt;


  NExtractMode::EEnum extractMode;
  bool isExtractGroupCommand = command.IsFromExtractGroup(extractMode);

  bool passwordEnabled = parser[NKey::kPassword].ThereIs;

  UString password;
  if(passwordEnabled)
    password = parser[NKey::kPassword].PostStrings[0];

  if(isExtractGroupCommand || command.CommandType == NCommandType::kList)
  {
    NFind::CFileInfo archiveFileInfo;
    if (!NFind::FindFile(archiveName, archiveFileInfo) || archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    UString defaultItemName;
    CMyComPtr<IInArchive> archiveHandler;
    CArchiverInfo archiverInfo;
    MyOpenArhive(archiveName, archiveFileInfo, &archiveHandler, defaultItemName);
    if(isExtractGroupCommand)
    {
      PrintProcessTitle(kExtractGroupProcessMessage, archiveName);
      CSysString outputDir;
      if(parser[NKey::kOutputDir].ThereIs)
      {
        outputDir = GetSystemString(parser[NKey::kOutputDir].PostStrings[0]);
        NName::NormalizeDirPathPrefix(outputDir);
      }
      NExtraction::NOverwriteMode::EEnum overwriteMode = 
          NExtraction::NOverwriteMode::kAskBefore;
      CExtractOptions options(extractMode, outputDir, yesToAll, 
          passwordEnabled, password, overwriteMode);
      options.DefaultItemName = defaultItemName;
      options.ArchiveFileInfo = archiveFileInfo;
      // options.ArchiveFileInfo = archiveFileInfo;
      HRESULT result = DeCompressArchiveSTD(archiveHandler, wildcardCensor, options);
      if (result != S_OK)
      {
        return NExitCode::kErrorsDuringDecompression;
      }
    }
    else
    {
      PrintProcessTitle(kListingProcessMessage, archiveName);
      ListArchive(archiveHandler, defaultItemName, archiveFileInfo, 
          wildcardCensor/*, command.ListFullPathes, command.ListMode*/);
    }
  }
  else
    PrintHelpAndExit();
  return 0;
}
