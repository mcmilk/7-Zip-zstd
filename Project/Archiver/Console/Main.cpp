// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "../../Crypto/Cipher/Common/CipherInterface.h"
#include "../../Compress/Interface/CompressInterface.h"
#include "../Format/Common/FormatCryptoInterface.h"

#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"

#include "../Common/CompressEngineCommon.h"
#include "../Common/UpdatePairBasic.h"
#include "../Common/OpenEngine200.h"

#include "../Common/ZipRegistryMain.h"
#include "../Common/DefaultName.h"


#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Registry.h"
#include "Windows/Defs.h"

#include "ListArchive.h"
#include "ExtractSTD.h"
#include "ArError.h"
#include "AddSTD.h"

#include "UpdateArchiveOptions.h"

using namespace NWindows;
using namespace NFile;
using namespace NComandLineParser;


static const char *kCopyrightString = "\n7-Zip"
#ifdef EXCLUDE_COM
" (Alone)"
#endif

" 2.30 Beta 8  Copyright (c) 1999-2001 Igor Pavlov  21-Dec-2001\n";

const char *kDefaultArchiveType = "7z";
const char *kDefaultSfxModule = "7zCon.sfx";
const LPCTSTR kSFXExtension = TEXT("exe");

static const int kNumSwitches = 15;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDisablePercents,
  kArchiveType,
  kYes,
  kPassword,
  kProperty,
  kOutputDir,
  kWorkingDir,
  kInclude,
  kExclude,
  kUpdate,
  kRecursed,
  kSfx,
  kOverwrite
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

static const char *kOverwritePostCharSet = "asu";

NZipSettings::NExtraction::NOverwriteMode::EEnum k_OverwriteModes[] =
{
  // NZipSettings::NExtraction::NOverwriteMode::kAskBefore,
  NZipSettings::NExtraction::NOverwriteMode::kWithoutPrompt,
  NZipSettings::NExtraction::NOverwriteMode::kSkipExisting,
  NZipSettings::NExtraction::NOverwriteMode::kAutoRename
};


static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    { "?",  NSwitchType::kSimple, false },
    { "H",  NSwitchType::kSimple, false },
    { "BD", NSwitchType::kSimple, false },
    { "T",  NSwitchType::kUnLimitedPostString, false, 1 },
    { "Y",  NSwitchType::kSimple, false },
    { "P",  NSwitchType::kUnLimitedPostString, false, 1 },
    { "M", NSwitchType::kUnLimitedPostString, true, 1 },
    { "O",  NSwitchType::kUnLimitedPostString, false, 1 },
    { "W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { "I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { "X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { "U",  NSwitchType::kUnLimitedPostString, true, 1},
    { "R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    { "SFX",  NSwitchType::kUnLimitedPostString, false, 0 },
    { "AO",  NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet}
  };

static const kNumCommandForms = 7;

namespace NCommandType {
enum EEnum
{
  kAdd = 0,
  kUpdate,
  kDelete,
  kTest,
  kExtract,
  kFullExtract,
  kList
};

}

static const CCommandForm aCommandForms[kNumCommandForms] = 
{
  { "A", false },
  { "U", false },
  { "D", false },
  { "T", false },
  { "E", false },
  { "X", false },
  { "L", false }
  // { "L", true }
};

static const NRecursedType::EEnum kCommandRecursedDefault[kNumCommandForms] = 
{
  NRecursedType::kNonRecursed, 
  NRecursedType::kNonRecursed, 
  NRecursedType::kNonRecursed, 
  NRecursedType::kRecursed,
  NRecursedType::kRecursed,
  NRecursedType::kRecursed,
  NRecursedType::kRecursed
};

/*
static const kNumListSets = 2;

enum 
{
  kListModeAddIndex     = 0,
  kListModeAllIndex     = 1
};

enum 
{
  kListModeSetIndex = 0, 
  kListFullPathSetIndex
};

static const CCommandSubCharsSet kListSets[kNumListSets] = 
{
  { "AT", true },
  { "F", true }
};
*/

// -------------------------------------------------
// Update area

//  OnlyOnDisk , OnlyInArchive, NewInArchive, OldInArchive,   
//  SameFiles, NotMasked
const CSysString kUpdatePairStateIDSet = "PQRXYZW";
const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

const CSysString kUpdatePairActionIDSet = "0123"; //Ignore, Copy, Compress

const CSysString kUpdateIgnoreItselfPostStringID = "-"; 
const char kUpdateNewArchivePostCharID = '!'; 

static const bool kTestExtractRecursedDefault = true;
static const bool kAddRecursedDefault = false;

static const kMaxCmdLineSize = 1000;
static const wchar_t *kUniversalWildcard = L"*";
static const kMinNonSwitchWords = 2;
static const kCommandIndex = 0;
static const kArchiveNameIndex = kCommandIndex + 1;
static const kFirstFileNameIndex = kArchiveNameIndex + 1;

static const char *kHelpString = 
    "\nUsage: 7z <command> [<switches>...] <archive_name> [<file_names>...]\n"
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
    "  -bd Disable percentage indicator\n"
    "  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
    "  -m{Parameters}: set compression Method\n"
//    "     -m0: store (no compression)\n"
//    "  -md<#>[b|k|m]: set Dictionary Size\n"
//    "  -mx: maXimize compression\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -r[-|0]: Recurse subdirectories\n"
    "  -sfx[{name}]: Create SFX archive\n"
    "  -t{Type}: Set type of archive\n"
    "  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName]: Update options\n"
    "  -w[{path}]: assign Work directory. Empty path means a temporary directory\n"
    "  -x[r[-|0]]]{@listfile|!wildcard}: eXclude filenames\n"
    "  -y: assume Yes on all queries\n";


// ---------------------------
// exception messages

static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError
static const char *kIncorrectListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInListFile = "Incorrect wildcard in listfile";
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
  bool IsFromUpdateGroup() const;
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
    case NCommandType::kExtract:
      aExtractMode = NExtractMode::kExtractToOne;
      return true;
    case NCommandType::kFullExtract:
      aExtractMode = NExtractMode::kFullPath;
      return true;
    default:
      return false;
  }
}

bool CArchiveCommand::IsFromUpdateGroup() const
{
  return (CommandType == NCommandType::kAdd || 
    CommandType == NCommandType::kUpdate ||
    CommandType == NCommandType::kDelete);
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
  if (aCommand.CommandType != NCommandType::kList)
    return true;

  /*
  CIntVector anIndexes;
  if(!ParseSubCharsCommand(kNumListSets, kListSets, aPostString, anIndexes))
    return false;
  switch (anIndexes[kListModeSetIndex])
  {
    case kListModeAddIndex:
      aCommand.ListMode = NListMode::kAdd;
      break;
    case kListModeAllIndex:
      aCommand.ListMode = NListMode::kAll;
      break;
    default:
      aCommand.ListMode = NListMode::kDefault;
      break;
  }
  aCommand.ListFullPathes = (anIndexes[kListFullPathSetIndex] >= 0);
  */
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


void AddToCensorFromListFile(NWildcard::CCensor &aWildcardCensor, 
    LPCTSTR aFileName, bool anInclude, NRecursedType::EEnum aType)
{
  AStringVector aNames;
  if (!ReadNamesFromListFile(aFileName, aNames))
    ShowMessageAndThrowException(kIncorrectListFile, NExitCode::kUserError);
  for (int i = 0; i < aNames.Size(); i++)
    if (!AddNameToCensor(aWildcardCensor, 
        MultiByteToUnicodeString(aNames[i], CP_OEMCP), anInclude, aType))
      ShowMessageAndThrowException(kIncorrectWildCardInListFile, NExitCode::kUserError);
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
  int aNumNonSwitchStrings = aNonSwitchStrings.Size();
  if(aNumNonSwitchStrings == kMinNonSwitchWords && (!aThereAreSwitchIncludeWildCards)) 
    AddCommandLineWildCardToCensr(aWildcardCensor, kUniversalWildcard, true, aType);
  for(int i = kFirstFileNameIndex; i < aNumNonSwitchStrings; i++)
  {
    const AString &aString = aNonSwitchStrings[i];
    if (AreEqualNoCase(aString[0], kFileListID))
      AddToCensorFromListFile(aWildcardCensor, aString.Mid(1), true, aType);
    else
      AddCommandLineWildCardToCensr(aWildcardCensor, 
      MultiByteToUnicodeString(aString, CP_OEMCP), true, aType);
  }
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
    else if (AreEqualNoCase(aString[aPos], kFileListID))
      AddToCensorFromListFile(aWildcardCensor, aTail, anInclude, aRecursedType);
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

// ------------------------------------------------------
// AddCommand functions

/*

// return NumDigits read: 0 - if it is not number;
int ParseNumberString(const CSysString &aString, int &aNumber)
{
  CSysString aNumberString;
  int i = 0;
  for(; i < aString.Length() && i < kMaxNumberOfDigitsInInputNumber; i++)
  {
    char aChar = aString[i];
    if(!isdigit(aChar))
      break;
    aNumberString += aChar;
  }
  if (i > 0)
    aNumber = atoi(aNumberString.GetPointer());
  return i;
}
*/

static NUpdateArchive::NPairAction::EEnum GetUpdatePairActionType(int i)
{
  switch(i)
  {
    case NUpdateArchive::NPairAction::kIgnore: return NUpdateArchive::NPairAction::kIgnore;
    case NUpdateArchive::NPairAction::kCopy: return NUpdateArchive::NPairAction::kCopy;
    case NUpdateArchive::NPairAction::kCompress: return NUpdateArchive::NPairAction::kCompress;
    case NUpdateArchive::NPairAction::kCompressAsAnti: return NUpdateArchive::NPairAction::kCompressAsAnti;
  }
  throw 98111603;
}

bool ParseUpdateCommandString2(const CSysString &aString, NUpdateArchive::CActionSet &anActionSet, 
    CSysString &aPostString)
{
  for(int i = 0; i < aString.Length();)
  {
    char aChar = toupper(aString[i]);
    int aStatePos = kUpdatePairStateIDSet.Find(aChar);
    if (aStatePos < 0)
    {
      aPostString = aString.Mid(i);
      return true;
    }
    i++;
    if (i >= aString.Length())
      return false;
    int anActionPos = kUpdatePairActionIDSet.Find(toupper(aString[i]));
    if (anActionPos < 0)
      return false;
    anActionSet.StateActions[aStatePos] = GetUpdatePairActionType(anActionPos);
    if (kUpdatePairStateNotSupportedActions[aStatePos] == anActionPos)
      return false;
    i++;
  }
  aPostString.Empty();
  return true;
}

CSysString MakeFullArchiveName(const CSysString &aName, const CSysString &anExtension)
{
  if (anExtension.IsEmpty())
    return aName;
  if (aName.IsEmpty())
    return aName;
  TCHAR aLastChar = *CharPrev(aName, ((LPCTSTR)(aName)) + aName.Length());   
  if (aLastChar == '.')
    return aName.Left(aName.Length() - 1);
  if (aName.Find(TCHAR('.')) >= 0)
    return aName;
  return aName + TCHAR('.') + anExtension;
}

void ParseUpdateCommandString(CUpdateArchiveOptions &anOptions, 
    const AStringVector &anUpdatePostStrings, 
    const NUpdateArchive::CActionSet &aDefaultActionSet,
    const CSysString &anExtension)
{
  for(int i = 0; i < anUpdatePostStrings.Size(); i++)
  {
    const CSysString &anUpdateString = anUpdatePostStrings[i];
    if(anUpdateString.CompareNoCase(kUpdateIgnoreItselfPostStringID) == 0)
    {
      if(anOptions.UpdateArchiveItself)
      {
        anOptions.UpdateArchiveItself = false;
        anOptions.Commands.Delete(0);
      }
    }
    else
    {
      NUpdateArchive::CActionSet anActionSet = aDefaultActionSet;

      CSysString aPostString;
      if (!ParseUpdateCommandString2(anUpdateString, anActionSet, aPostString))
        PrintHelpAndExit();
      if(aPostString.IsEmpty())
      {
        if(anOptions.UpdateArchiveItself)
        {
          anOptions.Commands[0].ActionSet = anActionSet;
        }
      }
      else
      {
        if(toupper(aPostString[0]) != kUpdateNewArchivePostCharID)
          PrintHelpAndExit();
        CUpdateArchiveCommand anUpdateCommand;

        /*
        if (!MakeArchiveNameWithExtension(aPostString.Mid(1),
            kArchiveTagExtension, anUpdateCommand.ArchivePath))
          PrintHelpAndExit();
        */

        anUpdateCommand.ArchivePath = aPostString.Mid(1);

        if (anUpdateCommand.ArchivePath.IsEmpty())
          PrintHelpAndExit();
        anUpdateCommand.ArchivePath = MakeFullArchiveName(anUpdateCommand.ArchivePath, anExtension);
        anUpdateCommand.ActionSet = anActionSet;
        anOptions.Commands.Add(anUpdateCommand);
      }
    }
  }
}

static void SetAddCommandOptions(NCommandType::EEnum aCommandType, 
    const NComandLineParser::CParser &aParser, const CSysString &anArchivePath, 
    CUpdateArchiveOptions &anOptions, CSysString &aWorkingDir, 
    const CSysString &anExtension)
{
  NUpdateArchive::CActionSet aDefaultActionSet;
  switch(aCommandType)
  {
    case NCommandType::kAdd: 
      aDefaultActionSet = kAddActionSet;
      break;
    case NCommandType::kDelete: 
      aDefaultActionSet = kDeleteActionSet;
      break;
    default: 
      aDefaultActionSet = kUpdateActionSet;
  }
  
  anOptions.ArchivePath = anArchivePath;
  anOptions.UpdateArchiveItself = true;
  
  anOptions.Commands.Clear();
  CUpdateArchiveCommand anUpdateMainCommand;
  anUpdateMainCommand.ActionSet = aDefaultActionSet;
  anOptions.Commands.Add(anUpdateMainCommand);
  // anOptions.ItselfActionSet = aDefaultActionSet;
  if(aParser[NKey::kUpdate].ThereIs)
    ParseUpdateCommandString(anOptions, aParser[NKey::kUpdate].PostStrings, 
        aDefaultActionSet, anExtension);
  if(aParser[NKey::kWorkingDir].ThereIs)
  {
    const CSysString &aPostString = aParser[NKey::kWorkingDir].PostStrings[0];
    if (aPostString.IsEmpty())
      NDirectory::MyGetTempPath(aWorkingDir);
    else
      aWorkingDir = aPostString;
  }
  else
  {
    if (!NDirectory::GetOnlyDirPrefix(anArchivePath, aWorkingDir))
      throw "bad archive name";
    if (aWorkingDir.IsEmpty())
      aWorkingDir = _T(".\\");
  }
  if(anOptions.SfxMode = aParser[NKey::kSfx].ThereIs)
  {
    CSysString aModuleName = aParser[NKey::kSfx].PostStrings[0];
    if (aModuleName.IsEmpty())
      aModuleName = kDefaultSfxModule;
    if (!NDirectory::MySearchPath(NULL, aModuleName, NULL, anOptions.SfxModule))
      throw "can't find specified sfx module";
  }
}


// static const kMinLogarithmicSize = 0;
static const kMaxLogarithmicSize = 31;

static const char kByteSymbol = 'B';
static const char kKiloByteSymbol = 'K';
static const char kMegaByteSymbol = 'M';

static const kNumDicts = 7;

static const kMaxNumberOfDigitsInInputNumber = 9;

static int ParseNumberString(const CSysString &aString, int &aNumber)
{
  CSysString aNumberString;
  int i = 0;
  for(; i < aString.Length() && i < kMaxNumberOfDigitsInInputNumber; i++)
  {
    char aChar = aString[i];
    if(!isdigit(aChar))
      break;
    aNumberString += aChar;
  }
  if (i > 0)
    aNumber = atoi(aNumberString);
  return i;
}

static void SetMethodOptions(const NComandLineParser::CParser &aParser, 
    CUpdateArchiveOptions &anOptions)
{
  if (aParser[NKey::kProperty].ThereIs)
  {
    // anOptions.MethodMode.Properties.Clear();
    for(int i = 0; i < aParser[NKey::kProperty].PostStrings.Size(); i++)
    {
      CProperty aProperty;
      const UString &aString = GetUnicodeString(aParser[NKey::kProperty].PostStrings[i]);
      int anIndex = aString.Find(L'=');
      if (anIndex < 0)
        aProperty.Name = aString;
      else
      {
        aProperty.Name = aString.Left(anIndex);
        aProperty.Value = aString.Mid(anIndex + 1);
      }
      anOptions.MethodMode.Properties.Add(aProperty);
    }
  }
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

#ifndef EXCLUDE_COM
void SetArchiveType(const CSysString &anArchiveType, CLSID &aClassID, CSysString &anArchiveExtension)
#else
void SetArchiveType(const CSysString &anArchiveType, CSysString &aFormatName, CSysString &anArchiveExtension)
#endif
{
  CObjectVector<NZipRootRegistry::CArchiverInfo> anArchiverInfoVector;
  NZipRootRegistry::ReadArchiverInfoList(anArchiverInfoVector);
  if (anArchiverInfoVector.Size() == 0)
    throw "There are no installed archive handlers";
  if (anArchiveType.IsEmpty())
  {
    throw "Incorrect archive type was assigned";
    // const NZipRootRegistry::CArchiverInfo &anArchiverInfo = anArchiverInfoVector[0];
    // aClassID = anArchiverInfo.ClassID;
    // return;
  }
  for (int i = 0; i < anArchiverInfoVector.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &anArchiverInfo = anArchiverInfoVector[i];
    if (anArchiverInfo.Name.CompareNoCase(anArchiveType) == 0)
    {
      #ifndef EXCLUDE_COM
      aClassID = anArchiverInfo.ClassID;
      #else
      aFormatName = anArchiverInfo.Name;
        
      #endif

      anArchiveExtension  = anArchiverInfo.Extension;
      return;
    }
  }
  throw "Incorrect archive type was assigned";
}



static const TCHAR *kCUBasePath = _T("Software\\7-ZIP");
static const TCHAR *kSwitchesValueName = _T("Switches");

CSysString GetDefaultSwitches()
{
  NRegistry::CKey aKey;
  if (aKey.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return CSysString();
  CSysString aString;
  if (aKey.QueryValue(kSwitchesValueName, aString) != ERROR_SUCCESS)
    return CSysString();
  return aString;
}

void SplitSpaceDelimetedStrings(const CSysString &aString, CSysStringVector &aStrings)
{
  aStrings.Clear();
  CSysString aCurrentString;
  for (int i = 0; i < aString.Length(); i++)
  {
    char aChar = aString[i];
    if (aChar == ' ')
    {
      if (!aCurrentString.IsEmpty())
      {
        aStrings.Add(aCurrentString);
        aCurrentString.Empty();
      }
    }
    else
      aCurrentString += aChar;
  }
  if (!aCurrentString.IsEmpty())
    aStrings.Add(aCurrentString);
}

int Main2(int aNumArguments, const char *anArguments[])
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;
  
  if(aNumArguments == 1)
  {
    PrintHelp();
    return 0;
  }
  AStringVector aCommandStrings;
  WriteArgumentsToStringList(aNumArguments, anArguments, aCommandStrings);

  NComandLineParser::CParser aDefaultSwitchesParser(kNumSwitches);
  try
  {
    CSysStringVector aDefaultSwitchesVector;
    SplitSpaceDelimetedStrings(GetDefaultSwitches(), aDefaultSwitchesVector);
    aDefaultSwitchesParser.ParseStrings(kSwitchForms, aDefaultSwitchesVector);
  }
  catch(...) 
  {
    PrintHelpAndExit();
  }


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
  if(aNumNonSwitchStrings < kMinNonSwitchWords)  
    PrintHelpAndExit();
  CArchiveCommand aCommand;
  if (!ParseArchiveCommand(aNonSwitchStrings[kCommandIndex], aCommand))
    PrintHelpAndExit();

  NRecursedType::EEnum aRecursedType;
  if (aParser[NKey::kRecursed].ThereIs)
    aRecursedType = GetRecursedTypeFromIndex(aParser[NKey::kRecursed].PostCharIndex);
  else
    aRecursedType = aCommand.DefaultRecursedType();

  NWildcard::CCensor aWildcardCensor;
  
  bool aThereAreSwitchIncludeWildCards;
  if (aParser[NKey::kInclude].ThereIs)
  {
    aThereAreSwitchIncludeWildCards = true;
    AddSwitchWildCardsToCensor(aWildcardCensor, aParser[NKey::kInclude].PostStrings, 
        true, aRecursedType);
  }
  else
    aThereAreSwitchIncludeWildCards = false;
  if (aParser[NKey::kExclude].ThereIs)
    AddSwitchWildCardsToCensor(aWildcardCensor, aParser[NKey::kExclude].PostStrings, 
        false, aRecursedType);
 
  AddToCensorFromNonSwitchesStrings(aWildcardCensor, aNonSwitchStrings, aRecursedType,
      aThereAreSwitchIncludeWildCards);


  bool anYesToAll = aParser[NKey::kYes].ThereIs;

  CSysString anArchiveName;
  /*
  if (!MakeArchiveNameWithExtension(aNonSwitchStrings[kArchiveNameIndex],
      kArchiveTagExtension, anArchiveName))
    PrintHelpAndExit();
  */
  anArchiveName = aNonSwitchStrings[kArchiveNameIndex];

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
      if(aParser[NKey::kOverwrite].ThereIs)
        anOverwriteMode = k_OverwriteModes[aParser[NKey::kOverwrite].PostCharIndex];
     

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
  else if(aCommand.IsFromUpdateGroup())
  {
    CUpdateArchiveOptions anOptions;
    CSysString aWorkingDir;

    CSysString anArchiveType;
    if(aParser[NKey::kArchiveType].ThereIs)
      anArchiveType = aParser[NKey::kArchiveType].PostStrings[0];
    else
      if(aDefaultSwitchesParser[NKey::kArchiveType].ThereIs)
        anArchiveType = aDefaultSwitchesParser[NKey::kArchiveType].PostStrings[0];
      else
        anArchiveType = kDefaultArchiveType;

    CSysString anExtension;
    if (!anArchiveType.IsEmpty())
    {
      #ifndef EXCLUDE_COM
      SetArchiveType(anArchiveType, anOptions.MethodMode.ClassID, anExtension);
      #else
      SetArchiveType(anArchiveType, anOptions.MethodMode.Name, anExtension);
      #endif
    }
    if(aParser[NKey::kSfx].ThereIs)
      anExtension = kSFXExtension;
    anArchiveName = MakeFullArchiveName(anArchiveName, anExtension);


    SetAddCommandOptions(aCommand.CommandType, aParser, anArchiveName, anOptions,
        aWorkingDir, anExtension); 
    
    SetMethodOptions(aDefaultSwitchesParser, anOptions); 
    SetMethodOptions(aParser, anOptions); 

    NFind::CFileInfo anArchiveFileInfo;
    CComPtr<IArchiveHandler200> anArchive;

    UString aDefaultItemName;
    if (NFind::FindFile(anArchiveName, anArchiveFileInfo))
    {
      if (anArchiveFileInfo.IsDirectory())
        throw "there is no such archive";
      MyOpenArhive(anArchiveName, anArchiveFileInfo, &anArchive, aDefaultItemName);
    }
    else
      if (anArchiveType.IsEmpty())
        throw "type of archive is not specified";
    bool anEnableParcents = !aParser[NKey::kDisablePercents].ThereIs;
    HRESULT aResult  = UpdateArchiveStdMain(aWildcardCensor, anOptions, aWorkingDir, 
        anArchive, &aDefaultItemName, &anArchiveFileInfo, anEnableParcents);
    if (aResult != S_OK)
      throw NExitCode::CSystemError(aResult);
    // ThrowPrintFileIsNotArchiveException(anArchiveName);
  }
  else 
    PrintHelpAndExit();
  return 0;
}
