// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "../../Crypto/Cipher/Common/CipherInterface.h"
#include "../../Compress/Interface/CompressInterface.h"
#include "Interface/CryptoInterface.h"

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
#include "OpenCallback.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;


static const char *kCopyrightString = "\n7-Zip"
#ifdef EXCLUDE_COM
" (A)"
#endif

#ifdef UNICODE
" [NT]"
#endif

" 2.30 Beta 27  Copyright (c) 1999-2003 Igor Pavlov  2003-01-24\n";

const LPCTSTR kDefaultArchiveType = _T("7z");
const LPCTSTR kDefaultSfxModule = TEXT("7zCon.sfx");
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
    { "P",  NSwitchType::kUnLimitedPostString, false, 0 },
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

static const CCommandForm g_CommandForms[kNumCommandForms] = 
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
const AString kUpdatePairStateIDSet = "PQRXYZW";
const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

const AString kUpdatePairActionIDSet = "0123"; //Ignore, Copy, Compress

const char *kUpdateIgnoreItselfPostStringID = "-"; 
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

static const AString kFileIsNotArchiveMessageBefore = "File \"";
static const AString kFileIsNotArchiveMessageAfter = "\" is not archive";

static const char *kProcessArchiveMessage = " archive: ";


// ---------------------------

static const AString kExtractGroupProcessMessage = "Processing";
static const AString kListingProcessMessage = "Listing";

static const AString kDefaultWorkingDirectory = "";  // test it maybemust be "."

struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
  // NListMode::EEnum ListMode;
  // bool ListFullPathes;
  NRecursedType::EEnum DefaultRecursedType() const;
  bool IsFromExtractGroup(NExtractMode::EEnum &extractMode) const;
  bool IsFromUpdateGroup() const;
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
    case NCommandType::kExtract:
      extractMode = NExtractMode::kExtractToOne;
      return true;
    case NCommandType::kFullExtract:
      extractMode = NExtractMode::kFullPath;
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

static void ShowMessageAndThrowException(LPCSTR message, NExitCode::EEnum code)
{
  g_StdOut << message << endl;
  throw code;
}

static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(const AString &processTitle, const CSysString &archiveName)
{
  g_StdOut << endl << processTitle << 
      kProcessArchiveMessage << GetOemString(archiveName) << endl << endl;
}

void WriteArgumentsToStringList(int numArguments, const char *arguments[], 
    AStringVector &strings)
{
  for(int i = 1; i < numArguments; i++)
  {
    char cmdLineBuffer[kMaxCmdLineSize];
    strcpy(cmdLineBuffer, arguments[i]);
    CharToOemA(cmdLineBuffer, cmdLineBuffer);
    if (strlen(cmdLineBuffer) > 0)
      strings.Add(cmdLineBuffer);
  }
}


bool ParseArchiveCommand(const AString &commandString, CArchiveCommand &command)
{
  AString commandStringUpper = commandString;
  commandStringUpper.MakeUpper();
  AString postString;
  int commandIndex = ParseCommand(kNumCommandForms, g_CommandForms, commandStringUpper, 
      postString) ;
  if (commandIndex < 0)
    return false;
  command.CommandType = (NCommandType::EEnum)commandIndex;
  if (command.CommandType != NCommandType::kList)
    return true;

  /*
  CIntVector indices;
  if(!ParseSubCharsCommand(kNumListSets, kListSets, postString, indices))
    return false;
  switch (indices[kListModeSetIndex])
  {
    case kListModeAddIndex:
      command.ListMode = NListMode::kAdd;
      break;
    case kListModeAllIndex:
      command.ListMode = NListMode::kAll;
      break;
    default:
      command.ListMode = NListMode::kDefault;
      break;
  }
  command.ListFullPathes = (indices[kListFullPathSetIndex] >= 0);
  */
  return true;
}

// ------------------------------------------------------------------
// filenames functions

static bool TestIsPathLegal(const UString &name)
{
  if (name.Length() == 0)
    return false;
  if (name[0] == L'\\' || name[0] == L'/')
    return false;
  if (name.Length() < 3)
    return true;
  if (name[1] == L':' && name[2] == L'\\')
    return false;
  return true;
}

static bool AddNameToCensor(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!TestIsPathLegal(name))
    throw "Can't use absolute paths";
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


void AddToCensorFromListFile(NWildcard::CCensor &wildcardCensor, 
    LPCTSTR fileName, bool include, NRecursedType::EEnum type)
{
  AStringVector names;
  if (!ReadNamesFromListFile(fileName, names))
    ShowMessageAndThrowException(kIncorrectListFile, NExitCode::kUserError);
  for (int i = 0; i < names.Size(); i++)
    if (!AddNameToCensor(wildcardCensor, 
        MultiByteToUnicodeString(names[i], CP_OEMCP), include, type))
      ShowMessageAndThrowException(kIncorrectWildCardInListFile, NExitCode::kUserError);
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!AddNameToCensor(wildcardCensor, name, include, type))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

static bool AreEqualNoCase(char aChar1, char aChar2)
{
  return (toupper(aChar1) == toupper(aChar2));
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &wildcardCensor, 
    const AStringVector &nonSwitchStrings, NRecursedType::EEnum type, 
    bool thereAreSwitchIncludeWildCards)
{
  int numNonSwitchStrings = nonSwitchStrings.Size();
  if(numNonSwitchStrings == kMinNonSwitchWords && (!thereAreSwitchIncludeWildCards)) 
    AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
  for(int i = kFirstFileNameIndex; i < numNonSwitchStrings; i++)
  {
    const AString &aString = nonSwitchStrings[i];
    if (AreEqualNoCase(aString[0], kFileListID))
      AddToCensorFromListFile(wildcardCensor, 
          GetSystemString(aString.Mid(1), CP_OEMCP), true, type);
    else
      AddCommandLineWildCardToCensr(wildcardCensor, 
      MultiByteToUnicodeString(aString, CP_OEMCP), true, type);
  }
}

void AddSwitchWildCardsToCensor(NWildcard::CCensor &wildcardCensor, 
    const AStringVector &strings, bool include, 
    NRecursedType::EEnum commonRecursedType)
{
  for(int i = 0; i < strings.Size(); i++)
  {
    const AString &name = strings[i];
    NRecursedType::EEnum recursedType;
    int pos = 0;
    if (name.Length() < kSomeCludePostStringMinSize)
      PrintHelpAndExit();
    if (AreEqualNoCase(name[pos], kRecursedIDChar))
    {
      pos++;
      int index = AString(kRecursedPostCharSet).Find(name[pos]);
      recursedType = GetRecursedTypeFromIndex(index);
      if (index >= 0)
        pos++;
    }
    else
      recursedType = commonRecursedType;
    if (name.Length() < pos + kSomeCludeAfterRecursedPostStringMinSize)
      PrintHelpAndExit();
    AString tail = name.Mid(pos + 1);
    if (AreEqualNoCase(name[pos], kImmediateNameID))
      AddCommandLineWildCardToCensr(wildcardCensor, 
          MultiByteToUnicodeString(tail, CP_OEMCP), include, recursedType);
    else if (AreEqualNoCase(name[pos], kFileListID))
      AddToCensorFromListFile(wildcardCensor, GetSystemString(tail, CP_OEMCP), include, recursedType);
    else
      PrintHelpAndExit();
  }
}

// ------------------------------------------------------------------
static void ThrowPrintFileIsNotArchiveException(const CSysString &fileName)
{
  AString message;
  message = kFileIsNotArchiveMessageBefore + GetOemString(fileName) + kFileIsNotArchiveMessageAfter;
  ShowMessageAndThrowException(message, NExitCode::kFileIsNotArchive);
}

// ------------------------------------------------------
// AddCommand functions

/*

// return NumDigits read: 0 - if it is not number;
int ParseNumberString(const CSysString &aString, int &number)
{
  CSysString numberString;
  int i = 0;
  for(; i < aString.Length() && i < kMaxNumberOfDigitsInInputNumber; i++)
  {
    char aChar = aString[i];
    if(!isdigit(aChar))
      break;
    numberString += aChar;
  }
  if (i > 0)
    number = atoi(numberString.GetPointer());
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

bool ParseUpdateCommandString2(const AString &command, 
    NUpdateArchive::CActionSet &actionSet, AString &postString)
{
  for(int i = 0; i < command.Length();)
  {
    char c = toupper(command[i]);
    int statePos = kUpdatePairStateIDSet.Find(c);
    if (statePos < 0)
    {
      postString = command.Mid(i);
      return true;
    }
    i++;
    if (i >= command.Length())
      return false;
    int actionPos = kUpdatePairActionIDSet.Find(toupper(command[i]));
    if (actionPos < 0)
      return false;
    actionSet.StateActions[statePos] = GetUpdatePairActionType(actionPos);
    if (kUpdatePairStateNotSupportedActions[statePos] == actionPos)
      return false;
    i++;
  }
  postString.Empty();
  return true;
}

UString MakeFullArchiveName(const UString &name, const UString &extension)
{
  if (extension.IsEmpty())
    return name;
  if (name.IsEmpty())
    return name;
  if (name[name.Length() - 1] == L'.')
    return name.Left(name.Length() - 1);
  int slash1Pos = name.ReverseFind(L'\\');
  int slash2Pos = name.ReverseFind(L'/');
  int slashPos = MyMax(slash1Pos, slash2Pos);
  int dotPos = name.ReverseFind(L'.');
  if (dotPos > 0 && (dotPos > slashPos || slashPos < 0))
    return name;
  return name + L'.' + extension;
}

void ParseUpdateCommandString(CUpdateArchiveOptions &options, 
    const AStringVector &updatePostStrings, 
    const NUpdateArchive::CActionSet &defaultActionSet,
    const UString &extension)
{
  for(int i = 0; i < updatePostStrings.Size(); i++)
  {
    const AString &updateString = updatePostStrings[i];
    if(updateString.CompareNoCase(kUpdateIgnoreItselfPostStringID) == 0)
    {
      if(options.UpdateArchiveItself)
      {
        options.UpdateArchiveItself = false;
        options.Commands.Delete(0);
      }
    }
    else
    {
      NUpdateArchive::CActionSet actionSet = defaultActionSet;

      AString postString;
      if (!ParseUpdateCommandString2(updateString, actionSet, postString))
        PrintHelpAndExit();
      if(postString.IsEmpty())
      {
        if(options.UpdateArchiveItself)
        {
          options.Commands[0].ActionSet = actionSet;
        }
      }
      else
      {
        if(toupper(postString[0]) != kUpdateNewArchivePostCharID)
          PrintHelpAndExit();
        CUpdateArchiveCommand updateCommand;

        /*
        if (!MakeArchiveNameWithExtension(postString.Mid(1),
            kArchiveTagExtension, updateCommand.ArchivePath))
          PrintHelpAndExit();
        */

        
        UString archivePathU = MultiByteToUnicodeString(
              postString.Mid(1), CP_OEMCP);

        if (archivePathU.IsEmpty())
          PrintHelpAndExit();
        updateCommand.ArchivePath = GetSystemString(
            MakeFullArchiveName(archivePathU, extension), CP_OEMCP);
        updateCommand.ActionSet = actionSet;
        options.Commands.Add(updateCommand);
      }
    }
  }
}

static void SetAddCommandOptions(NCommandType::EEnum commandType, 
    const CParser &parser, 
    const CSysString &archivePath, 
    CUpdateArchiveOptions &options, CSysString &workingDir, 
    const UString &extension)
{
  NUpdateArchive::CActionSet defaultActionSet;
  switch(commandType)
  {
    case NCommandType::kAdd: 
      defaultActionSet = kAddActionSet;
      break;
    case NCommandType::kDelete: 
      defaultActionSet = kDeleteActionSet;
      break;
    default: 
      defaultActionSet = kUpdateActionSet;
  }
  
  options.ArchivePath = archivePath;
  options.UpdateArchiveItself = true;
  
  options.Commands.Clear();
  CUpdateArchiveCommand updateMainCommand;
  updateMainCommand.ActionSet = defaultActionSet;
  options.Commands.Add(updateMainCommand);
  // options.ItselfActionSet = defaultActionSet;
  if(parser[NKey::kUpdate].ThereIs)
    ParseUpdateCommandString(options, parser[NKey::kUpdate].PostStrings, 
        defaultActionSet, extension);
  if(parser[NKey::kWorkingDir].ThereIs)
  {
    const AString &postString = parser[NKey::kWorkingDir].PostStrings[0];
    if (postString.IsEmpty())
      NDirectory::MyGetTempPath(workingDir);
    else
      workingDir = GetSystemString(postString, CP_OEMCP);
  }
  else
  {
    if (!NDirectory::GetOnlyDirPrefix(archivePath, workingDir))
      throw "bad archive name";
    if (workingDir.IsEmpty())
      workingDir = _T(".\\");
  }
  if(options.SfxMode = parser[NKey::kSfx].ThereIs)
  {
    CSysString moduleName = GetSystemString(parser[NKey::kSfx].PostStrings[0], CP_OEMCP);
    if (moduleName.IsEmpty())
      moduleName = kDefaultSfxModule;
    if (!NDirectory::MySearchPath(NULL, moduleName, NULL, options.SfxModule))
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

static int ParseNumberString(const AString &srcString, int &number)
{
  AString numberString;
  int i = 0;
  for(; i < srcString.Length() && i < kMaxNumberOfDigitsInInputNumber; i++)
  {
    char c = srcString[i];
    if(!isdigit(c))
      break;
    numberString += c;
  }
  if (i > 0)
    number = atoi(numberString);
  return i;
}

static void SetMethodOptions(const CParser &parser, 
    CUpdateArchiveOptions &options)
{
  if (parser[NKey::kProperty].ThereIs)
  {
    // options.MethodMode.Properties.Clear();
    for(int i = 0; i < parser[NKey::kProperty].PostStrings.Size(); i++)
    {
      CProperty property;
      const UString &postString = GetUnicodeString(parser[NKey::kProperty].PostStrings[i]);
      int index = postString.Find(L'=');
      if (index < 0)
        property.Name = postString;
      else
      {
        property.Name = postString.Left(index);
        property.Value = postString.Mid(index + 1);
      }
      options.MethodMode.Properties.Add(property);
    }
  }
}

static void MyOpenArhive(const CSysString &archiveName, 
  const NFind::CFileInfo &archiveFileInfo,
  IInArchive **archiveHandler,
  UString &defaultItemName,
  bool &passwordEnabled, 
  UString &password)
{
  CComObjectNoLock<COpenCallbackImp> *openCallbackSpec = 
    new CComObjectNoLock<COpenCallbackImp>;
  CComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  if (passwordEnabled)
  {
    openCallbackSpec->PasswordIsDefined = passwordEnabled;
    openCallbackSpec->Password = password;
  }

  CSysString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  NZipRootRegistry::CArchiverInfo archiverInfo;
  HRESULT result = OpenArchive(archiveName, archiveHandler, archiverInfo, 
      openCallback);
  if (result == S_FALSE)
    throw "file is not supported archive";
  if (result != S_OK)
    throw "error";
  defaultItemName = GetDefaultName(archiveName, archiverInfo.Extension, 
      GetUnicodeString(archiverInfo.AddExtension));
  passwordEnabled = openCallbackSpec->PasswordIsDefined;
  password = openCallbackSpec->Password;
}

#ifndef EXCLUDE_COM
void SetArchiveType(const CSysString &archiveType, CLSID &classID, CSysString &archiveExtension)
#else
void SetArchiveType(const CSysString &archiveType, 
    CSysString &formatName, CSysString &archiveExtension)
#endif
{
  CObjectVector<NZipRootRegistry::CArchiverInfo> archiverInfoVector;
  NZipRootRegistry::ReadArchiverInfoList(archiverInfoVector);
  if (archiverInfoVector.Size() == 0)
    throw "There are no installed archive handlers";
  if (archiveType.IsEmpty())
  {
    throw "Incorrect archive type was assigned";
    // const NZipRootRegistry::CArchiverInfo &archiverInfo = archiverInfoVector[0];
    // classID = archiverInfo.ClassID;
    // return;
  }
  for (int i = 0; i < archiverInfoVector.Size(); i++)
  {
    const NZipRootRegistry::CArchiverInfo &archiverInfo = archiverInfoVector[i];
    if (archiverInfo.Name.CompareNoCase(archiveType) == 0)
    {
      #ifndef EXCLUDE_COM
      classID = archiverInfo.ClassID;
      #else
      formatName = archiverInfo.Name;
        
      #endif

      archiveExtension  = archiverInfo.Extension;
      return;
    }
  }
  throw "Incorrect archive type was assigned";
}



static const TCHAR *kCUBasePath = _T("Software\\7-ZIP");
static const TCHAR *kSwitchesValueName = _T("Switches");

AString GetDefaultSwitches()
{
  NRegistry::CKey key;
  if (key.Open(HKEY_CURRENT_USER, kCUBasePath, KEY_READ) != ERROR_SUCCESS)
    return AString();
  CSysString switchesString;
  if (key.QueryValue(kSwitchesValueName, switchesString) != ERROR_SUCCESS)
    return AString();
  #ifdef _UNICODE
  return GetOemString(switchesString);
  #else
  char cmdLineBuffer[kMaxCmdLineSize];
  strcpy(cmdLineBuffer, switchesString);
  CharToOemA(cmdLineBuffer, cmdLineBuffer);
  return cmdLineBuffer;
  #endif
}


void SplitSpaceDelimetedStrings(const AString &srcString, 
    AStringVector &destStrings)
{
  destStrings.Clear();
  AString currentString;
  for (int i = 0; i < srcString.Length(); i++)
  {
    char c = srcString[i];
    if (c == ' ')
    {
      if (!currentString.IsEmpty())
      {
        destStrings.Add(currentString);
        currentString.Empty();
      }
    }
    else
      currentString += c;
  }
  if (!currentString.IsEmpty())
    destStrings.Add(currentString);
}

int Main2(int numArguments, const char *arguments[])
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;
  
  if(numArguments == 1)
  {
    PrintHelp();
    return 0;
  }
  AStringVector commandStrings;
  WriteArgumentsToStringList(numArguments, arguments, commandStrings);

  CParser defaultSwitchesParser(kNumSwitches);
  try
  {
    AStringVector defaultSwitchesVector;
    SplitSpaceDelimetedStrings(GetDefaultSwitches(), defaultSwitchesVector);
    defaultSwitchesParser.ParseStrings(kSwitchForms, defaultSwitchesVector);
  }
  catch(...) 
  {
    PrintHelpAndExit();
  }


  CParser parser(kNumSwitches);
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
  const AStringVector &nonSwitchStrings = parser._nonSwitchStrings;

  int numNonSwitchStrings = nonSwitchStrings.Size();
  if(numNonSwitchStrings < kMinNonSwitchWords)  
    PrintHelpAndExit();
  CArchiveCommand command;
  if (!ParseArchiveCommand(nonSwitchStrings[kCommandIndex], command))
    PrintHelpAndExit();

  NRecursedType::EEnum recursedType;
  if (parser[NKey::kRecursed].ThereIs)
    recursedType = GetRecursedTypeFromIndex(parser[NKey::kRecursed].PostCharIndex);
  else
    recursedType = command.DefaultRecursedType();

  NWildcard::CCensor wildcardCensor;
  
  bool thereAreSwitchIncludeWildCards;
  if (parser[NKey::kInclude].ThereIs)
  {
    thereAreSwitchIncludeWildCards = true;
    AddSwitchWildCardsToCensor(wildcardCensor, parser[NKey::kInclude].PostStrings, 
        true, recursedType);
  }
  else
    thereAreSwitchIncludeWildCards = false;
  if (parser[NKey::kExclude].ThereIs)
    AddSwitchWildCardsToCensor(wildcardCensor, parser[NKey::kExclude].PostStrings, 
        false, recursedType);
 
  AddToCensorFromNonSwitchesStrings(wildcardCensor, nonSwitchStrings, recursedType,
      thereAreSwitchIncludeWildCards);


  bool yesToAll = parser[NKey::kYes].ThereIs;

  CSysString archiveName;
  /*
  if (!MakeArchiveNameWithExtension(nonSwitchStrings[kArchiveNameIndex],
      kArchiveTagExtension, archiveName))
    PrintHelpAndExit();
  */
  archiveName = GetSystemString(nonSwitchStrings[kArchiveNameIndex], CP_OEMCP);

  NExtractMode::EEnum extractMode;
  bool isExtractGroupCommand = command.IsFromExtractGroup(extractMode);

  bool passwordEnabled = parser[NKey::kPassword].ThereIs;

  UString password;
  if(passwordEnabled)
  {
    AString oemPassword = parser[NKey::kPassword].PostStrings[0]; // test this DirPath
    password = MultiByteToUnicodeString(oemPassword, CP_OEMCP);
  }

  if(isExtractGroupCommand || command.CommandType == NCommandType::kList)
  {
    NFind::CFileInfo archiveFileInfo;
    if (!NFind::FindFile(archiveName, archiveFileInfo) || archiveFileInfo.IsDirectory())
      throw "there is no such archive";

   if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    UString defaultItemName;
    CComPtr<IInArchive> archiveHandler;
    NZipRootRegistry::CArchiverInfo archiverInfo;

    MyOpenArhive(archiveName, archiveFileInfo, &archiveHandler, 
        defaultItemName, passwordEnabled, password);

    if(isExtractGroupCommand)
    {
      PrintProcessTitle(kExtractGroupProcessMessage, archiveName);
      CSysString outputDir;
      if(parser[NKey::kOutputDir].ThereIs)
      {
        outputDir = GetSystemString(parser[NKey::kOutputDir].PostStrings[0], CP_OEMCP); // test this DirPath
        NName::NormalizeDirPathPrefix(outputDir);
      }

      NZipSettings::NExtraction::NOverwriteMode::EEnum overwriteMode = 
          NZipSettings::NExtraction::NOverwriteMode::kAskBefore;
      if(parser[NKey::kOverwrite].ThereIs)
        overwriteMode = k_OverwriteModes[parser[NKey::kOverwrite].PostCharIndex];
     

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
  else if(command.IsFromUpdateGroup())
  {
    CUpdateArchiveOptions options;

    options.MethodMode.PasswordIsDefined = passwordEnabled && !password.IsEmpty();
    options.MethodMode.AskPassword = passwordEnabled && password.IsEmpty();
    options.MethodMode.Password = password;

    CSysString workingDir;

    CSysString archiveType;
    if(parser[NKey::kArchiveType].ThereIs)
      archiveType = GetSystemString(
          parser[NKey::kArchiveType].PostStrings[0], CP_OEMCP);
    else
      if(defaultSwitchesParser[NKey::kArchiveType].ThereIs)
        archiveType = GetSystemString(
            defaultSwitchesParser[NKey::kArchiveType].PostStrings[0], CP_OEMCP);
      else
        archiveType = kDefaultArchiveType;

    CSysString extension;
    if (!archiveType.IsEmpty())
    {
      #ifndef EXCLUDE_COM
      SetArchiveType(archiveType, options.MethodMode.ClassID, extension);
      #else
      SetArchiveType(archiveType, options.MethodMode.Name, extension);
      #endif
    }
    if(parser[NKey::kSfx].ThereIs)
      extension = kSFXExtension;
    UString extensionU = GetUnicodeString(extension);
    archiveName = 
        GetSystemString(
        MakeFullArchiveName(GetUnicodeString(archiveName, CP_OEMCP), 
        extensionU), CP_OEMCP);


    SetAddCommandOptions(command.CommandType, parser, archiveName, options,
        workingDir, extensionU); 
    
    SetMethodOptions(defaultSwitchesParser, options); 
    SetMethodOptions(parser, options); 

    if (options.SfxMode)
    {
      CProperty property;
      property.Name = L"rsfx";
      property.Value = L"on";
      options.MethodMode.Properties.Add(property);
    }

    NFind::CFileInfo archiveFileInfo;
    CComPtr<IInArchive> archive;

    UString defaultItemName;
    if (NFind::FindFile(archiveName, archiveFileInfo))
    {
      if (archiveFileInfo.IsDirectory())
        throw "there is no such archive";
      MyOpenArhive(archiveName, archiveFileInfo, &archive, 
          defaultItemName, passwordEnabled, password);
    }
    else
      if (archiveType.IsEmpty())
        throw "type of archive is not specified";
    bool enableParcents = !parser[NKey::kDisablePercents].ThereIs;
    HRESULT result  = UpdateArchiveStdMain(wildcardCensor, options, workingDir, 
        archive, &defaultItemName, &archiveFileInfo, enableParcents);
    if (result != S_OK)
      throw NExitCode::CSystemError(result);
    // ThrowPrintFileIsNotArchiveException(archiveName);
  }
  else 
    PrintHelpAndExit();
  return 0;
}
