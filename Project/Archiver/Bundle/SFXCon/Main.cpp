// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"

#include "../../Common/OpenEngine200.h"
#include "../../Common/ZipRegistryMain.h"
#include "../../Common/DefaultName.h"

#include "../../Console/ExtractSTD.h"
#include "../../Console/ArError.h"
#include "../../Console/ListArchive.h"

#include "../../Format/Common/FormatCryptoInterface.h"
#include "../../../Compress/Interface/CompressInterface.h"

using namespace NWindows;
using namespace NFile;
using namespace NComandLineParser;

static const char *kCopyrightString = 
"\n7-Zip SFX 2.30 Beta 13  Copyright (c) 1999-2002 Igor Pavlov  2002-01-31\n";

static const int kNumSwitches = 6;

const LPTSTR aDefaultExt = TEXT(".exe");

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
    { "?",  NSwitchType::kSimple, false },
    { "H",  NSwitchType::kSimple, false },
    { "BD", NSwitchType::kSimple, false },
    { "Y",  NSwitchType::kSimple, false },
    { "P",  NSwitchType::kUnLimitedPostString, false, 1 },
    { "O",  NSwitchType::kUnLimitedPostString, false, 1 },
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

static const CCommandForm aCommandForms[kNumCommandForms] = 
{
  { "T", false },
  // { "E", false },
  { "X", false },
  { "L", false }
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
  bool IsFromExtractGroup(NExtractMode::EEnum &aExtractMode) const;
};

NRecursedType::EEnum CArchiveCommand::DefaultRecursedType() const
{
  return kCommandRecursedDefault[CommandType];
}

bool CArchiveCommand::IsFromExtractGroup(NExtractMode::EEnum &aExtractMode) const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
      aExtractMode = NExtractMode::kTest;
      return true;
    /*
    case NCommandType::kExtract:
      aExtractMode = NExtractMode::kExtractToOne;
      return true;
    */
    case NCommandType::kFullExtract:
      aExtractMode = NExtractMode::kFullPath;
      return true;
    default:
      return false;
  }
}

static NRecursedType::EEnum GetRecursedTypeFromIndex(int anIndex)
{
  switch (anIndex)
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

static void ShowMessageAndThrowException(LPCTSTR aMessage, NExitCode::EEnum aCode)
{
  g_StdOut << aMessage << endl;
  throw aCode;
}

static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(const CSysString &aProcessTitle, const CSysString &anArchiveName)
{
  g_StdOut << endl << aProcessTitle << kProcessArchiveMessage << anArchiveName << endl << endl;
}

void WriteArgumentsToStringList(int aNumArguments, const char *anArguments[], 
    CSysStringVector &aStrings)
{
  for(int i = 1; i < aNumArguments; i++)
  {
    char aCmdLineBuffer[kMaxCmdLineSize];
    strcpy(aCmdLineBuffer, anArguments[i]);
    CharToOem(aCmdLineBuffer, aCmdLineBuffer);
    if (strlen(aCmdLineBuffer) > 0)
      aStrings.Add(aCmdLineBuffer);
  }
}


bool ParseArchiveCommand(const CSysString &aCommandString, CArchiveCommand &aCommand)
{
  CSysString aCommandStringUpper = aCommandString;
  aCommandStringUpper.MakeUpper();
  CSysString aPostString;
  int anCommandIndex = ParseCommand(kNumCommandForms, aCommandForms, aCommandStringUpper, 
      aPostString) ;
  if (anCommandIndex < 0)
    return false;
  aCommand.CommandType = (NCommandType::EEnum)anCommandIndex;
  return true;
}

// ------------------------------------------------------------------
// filenames functions

static bool AddNameToCensor(NWildcard::CCensor &aWildcardCensor, 
    const UString &aName, bool anInclude, NRecursedType::EEnum aType)
{
  /*
  if(!IsWildCardFilePathLegal(aName))
    return false;
  */
  bool anIsWildCard = DoesNameContainWildCard(aName);
  bool aRecursed;

  switch (aType)
  {
    case NRecursedType::kWildCardOnlyRecursed:
      aRecursed = anIsWildCard;
      break;
    case NRecursedType::kRecursed:
      aRecursed = true;
      break;
    case NRecursedType::kNonRecursed:
      aRecursed = false;
      break;
  }
  aWildcardCensor.AddItem(aName, anInclude, aRecursed, anIsWildCard);
  return true;
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &aWildcardCensor, 
    const UString &aName, bool anInclude, NRecursedType::EEnum aType)
{
  if (!AddNameToCensor(aWildcardCensor, aName, anInclude, aType))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

static bool AreEqualNoCase(char aChar1, char aChar2)
{
  return (toupper(aChar1) == toupper(aChar2));
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &aWildcardCensor, 
    const AStringVector &aNonSwitchStrings, NRecursedType::EEnum aType, 
    bool aThereAreSwitchIncludeWildCards)
{
  /*
  int aNumNonSwitchStrings = aNonSwitchStrings.Size();
  if(aNumNonSwitchStrings == kMinNonSwitchWords && (!aThereAreSwitchIncludeWildCards)) 
  */
    AddCommandLineWildCardToCensr(aWildcardCensor, kUniversalWildcard, true, aType);
  /*
  for(int i = kFirstFileNameIndex; i < aNumNonSwitchStrings; i++)
  {
    const AString &aString = aNonSwitchStrings[i];
    AddCommandLineWildCardToCensr(aWildcardCensor, 
      MultiByteToUnicodeString(aString, CP_OEMCP), true, aType);
  }
  */
}

void AddSwitchWildCardsToCensor(NWildcard::CCensor &aWildcardCensor, 
    const AStringVector &aStrings, bool anInclude, NRecursedType::EEnum aCommonRecursedType)
{
  for(int i = 0; i < aStrings.Size(); i++)
  {
    const AString &aString = aStrings[i];
    NRecursedType::EEnum aRecursedType;
    int aPos = 0;
    if (aString.Length() < kSomeCludePostStringMinSize)
      PrintHelpAndExit();
    if (AreEqualNoCase(aString[aPos], kRecursedIDChar))
    {
      aPos++;
      int anIndex = CSysString(kRecursedPostCharSet).Find(aString[aPos]);
      aRecursedType = GetRecursedTypeFromIndex(anIndex);
      if (anIndex >= 0)
        aPos++;
    }
    else
      aRecursedType = aCommonRecursedType;
    if (aString.Length() < aPos + kSomeCludeAfterRecursedPostStringMinSize)
      PrintHelpAndExit();
    AString aTail = aString.Mid(aPos + 1);
    if (AreEqualNoCase(aString[aPos], kImmediateNameID))
      AddCommandLineWildCardToCensr(aWildcardCensor, 
          MultiByteToUnicodeString(aTail, CP_OEMCP), anInclude, aRecursedType);
    else 
      PrintHelpAndExit();
  }
}

// ------------------------------------------------------------------
static void ThrowPrintFileIsNotArchiveException(const CSysString &aFileName)
{
  CSysString aMessage;
  aMessage = kFileIsNotArchiveMessageBefore + aFileName + kFileIsNotArchiveMessageAfter;
  ShowMessageAndThrowException(aMessage, NExitCode::kFileIsNotArchive);
}

void MyOpenArhive(const CSysString &anArchiveName, 
  const NFind::CFileInfo &anArchiveFileInfo,
  IArchiveHandler200 **anArchiveHandler,
  UString &aDefaultItemName)
{
  NZipRootRegistry::CArchiverInfo anArchiverInfo;
  HRESULT aResult  = OpenArchive(anArchiveName, anArchiveHandler, anArchiverInfo);
  if (aResult == S_FALSE)
    throw "file is not supported archive";
  if (aResult != S_OK)
    throw "error";
  aDefaultItemName = GetDefaultName(anArchiveName, anArchiverInfo.Extension);
}

int Main2(int aNumArguments, const char *anArguments[])
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;
  
  /*
  if(aNumArguments == 1)
  {
    PrintHelp();
    return 0;
  }
  */
  AStringVector aCommandStrings;
  WriteArgumentsToStringList(aNumArguments, anArguments, aCommandStrings);

  NComandLineParser::CParser aParser(kNumSwitches);
  try
  {
    aParser.ParseStrings(kSwitchForms, aCommandStrings);
  }
  catch(...) 
  {
    PrintHelpAndExit();
  }

  if(aParser[NKey::kHelp1].ThereIs || aParser[NKey::kHelp2].ThereIs)
  {
    PrintHelp();
    return 0;
  }
  const AStringVector &aNonSwitchStrings = aParser.m_NonSwitchStrings;

  int aNumNonSwitchStrings = aNonSwitchStrings.Size();
  /*
  if(aNumNonSwitchStrings < kMinNonSwitchWords)  
    PrintHelpAndExit();
  */
  CArchiveCommand aCommand;
  if (aNumNonSwitchStrings == 0)
    aCommand.CommandType = NCommandType::kFullExtract;
  else 
  {
    if (aNumNonSwitchStrings > 1)
      PrintHelpAndExit();
    if (!ParseArchiveCommand(aNonSwitchStrings[kCommandIndex], aCommand))
      PrintHelpAndExit();
  }


  NRecursedType::EEnum aRecursedType;
  aRecursedType = aCommand.DefaultRecursedType();

  NWildcard::CCensor aWildcardCensor;
  
  bool aThereAreSwitchIncludeWildCards;
  aThereAreSwitchIncludeWildCards = false;
  AddToCensorFromNonSwitchesStrings(aWildcardCensor, aNonSwitchStrings, aRecursedType,
      aThereAreSwitchIncludeWildCards);


  bool anYesToAll = aParser[NKey::kYes].ThereIs;

  CSysString anArchiveName;
  
  // anArchiveName = aNonSwitchStrings[kArchiveNameIndex];
  anArchiveName = anArguments[0];
  
  // g_StdOut << anArchiveName << endl;
  // g_StdOut << GetCommandLine() << endl;

  if (anArchiveName.Right(4).CompareNoCase(aDefaultExt) != 0)
    anArchiveName += aDefaultExt;


  NExtractMode::EEnum aExtractMode;
  bool anIsExtractGroupCommand = aCommand.IsFromExtractGroup(aExtractMode);

  bool aPasswordEnabled = aParser[NKey::kPassword].ThereIs;

  UString aPassword;
  if(aPasswordEnabled)
  {
    AString anOemPassword = aParser[NKey::kPassword].PostStrings[0]; // test this DirPath
    aPassword = MultiByteToUnicodeString(anOemPassword, CP_OEMCP);
  }

  if(anIsExtractGroupCommand || aCommand.CommandType == NCommandType::kList)
  {
    NFind::CFileInfo anArchiveFileInfo;
    if (!NFind::FindFile(anArchiveName, anArchiveFileInfo) || anArchiveFileInfo.IsDirectory())
      throw "there is no such archive";

    if (anArchiveFileInfo.IsDirectory())
      throw "there is no such archive";

    UString aDefaultItemName;
    CComPtr<IArchiveHandler200> anArchiveHandler;
    NZipRootRegistry::CArchiverInfo anArchiverInfo;
    MyOpenArhive(anArchiveName, anArchiveFileInfo, &anArchiveHandler, aDefaultItemName);
    if(anIsExtractGroupCommand)
    {
      PrintProcessTitle(kExtractGroupProcessMessage, anArchiveName);
      CSysString anOutputDir;
      if(aParser[NKey::kOutputDir].ThereIs)
      {
        anOutputDir = aParser[NKey::kOutputDir].PostStrings[0]; // test this DirPath
        NName::NormalizeDirPathPrefix(anOutputDir);
      }
      NZipSettings::NExtraction::NOverwriteMode::EEnum anOverwriteMode = 
          NZipSettings::NExtraction::NOverwriteMode::kAskBefore;
      CExtractOptions anOptions(aExtractMode, anOutputDir, anYesToAll, 
          aPasswordEnabled, aPassword, anOverwriteMode);
      anOptions.DefaultItemName = aDefaultItemName;
      anOptions.ArchiveFileInfo = anArchiveFileInfo;
      // anOptions.ArchiveFileInfo = anArchiveFileInfo;
      HRESULT aResult = DeCompressArchiveSTD(anArchiveHandler, aWildcardCensor, anOptions);
      if (aResult != S_OK)
      {
        return NExitCode::kErrorsDuringDecompression;
      }
    }
    else
    {
      PrintProcessTitle(kListingProcessMessage, anArchiveName);
      ListArchive(anArchiveHandler, aDefaultItemName, anArchiveFileInfo, 
          aWildcardCensor/*, aCommand.ListFullPathes, aCommand.ListMode*/);
    }
  }
  else
    PrintHelpAndExit();
  return 0;
}
