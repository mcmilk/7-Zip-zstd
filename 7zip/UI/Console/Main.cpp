// Main.cpp

#include "StdAfx.h"

#include <initguid.h>
#include <io.h>


#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"

#include "../../IPassword.h"
#include "../../ICoder.h"
#include "../../Compress/LZ/IMatchFinder.h"
#include "../Common/DefaultName.h"
#include "../Common/OpenArchive.h"
#include "../Common/ArchiverInfo.h"
#include "../Common/UpdateAction.h"

#include "List.h"
#include "Extract.h"
#include "Update.h"
#include "ArError.h"
#include "OpenCallback.h"

#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

HINSTANCE g_hInstance = 0;

static const char *kCopyrightString = "\n7-Zip"
#ifdef EXCLUDE_COM
" (A)"
#endif

#ifdef UNICODE
" [NT]"
#endif

" 3.12  Copyright (c) 1999-2003 Igor Pavlov  2003-12-10\n";

const wchar_t *kDefaultArchiveType = L"7z";
const wchar_t *kDefaultSfxModule = L"7zCon.sfx";
const wchar_t *kSFXExtension = L"exe";

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

static const wchar_t kRecursedIDChar = 'R';
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

static const wchar_t *kOverwritePostCharSet = L"asut";

NExtraction::NOverwriteMode::EEnum k_OverwriteModes[] =
{
  NExtraction::NOverwriteMode::kWithoutPrompt,
  NExtraction::NOverwriteMode::kSkipExisting,
  NExtraction::NOverwriteMode::kAutoRename,
  NExtraction::NOverwriteMode::kAutoRenameExisting
};


static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"T",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"Y",  NSwitchType::kSimple, false },
    { L"P",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"M", NSwitchType::kUnLimitedPostString, true, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"U",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    { L"SFX",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"AO",  NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet}
  };

static const int kNumCommandForms = 7;

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
  { L"A", false },
  { L"U", false },
  { L"D", false },
  { L"T", false },
  { L"E", false },
  { L"X", false },
  { L"L", false }
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


// -------------------------------------------------
// Update area

const UString kUpdatePairStateIDSet = L"PQRXYZW";
const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

const UString kUpdatePairActionIDSet = L"0123"; //Ignore, Copy, Compress

const wchar_t *kUpdateIgnoreItselfPostStringID = L"-"; 
const wchar_t kUpdateNewArchivePostCharID = '!'; 

static const bool kTestExtractRecursedDefault = true;
static const bool kAddRecursedDefault = false;

static const int kMaxCmdLineSize = 1000;
static const wchar_t *kUniversalWildcard = L"*";
static const int kMinNonSwitchWords = 2;
static const int kCommandIndex = 0;
static const int kArchiveNameIndex = kCommandIndex + 1;
static const int kFirstFileNameIndex = kArchiveNameIndex + 1;

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

static const char *kProcessArchiveMessage = " archive: ";

// ---------------------------

static const AString kExtractGroupProcessMessage = "Processing";
static const AString kListingProcessMessage = "Listing";

static const AString kDefaultWorkingDirectory = "";  // test it maybemust be "."

struct CArchiveCommand
{
  NCommandType::EEnum CommandType;
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

static void PrintProcessTitle(const AString &processTitle, const UString &archiveName)
{
  g_StdOut << endl << processTitle << 
      kProcessArchiveMessage << archiveName << endl << endl;
}

bool ParseArchiveCommand(const UString &commandString, CArchiveCommand &command)
{
  UString commandStringUpper = commandString;
  commandStringUpper.MakeUpper();
  UString postString;
  int commandIndex = ParseCommand(kNumCommandForms, g_CommandForms, commandStringUpper, 
      postString) ;
  if (commandIndex < 0)
    return false;
  command.CommandType = (NCommandType::EEnum)commandIndex;
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

static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 

void AddToCensorFromListFile(NWildcard::CCensor &wildcardCensor, 
    LPCWSTR fileName, bool include, NRecursedType::EEnum type)
{
  UStringVector names;
  if (!ReadNamesFromListFile(GetSystemString(fileName, 
        GetCurrentCodePage()), names))
    ShowMessageAndThrowException(kIncorrectListFile, NExitCode::kUserError);
  for (int i = 0; i < names.Size(); i++)
    if (!AddNameToCensor(wildcardCensor, names[i], include, type))
      ShowMessageAndThrowException(kIncorrectWildCardInListFile, NExitCode::kUserError);
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum type)
{
  if (!AddNameToCensor(wildcardCensor, name, include, type))
    ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
}

void AddToCensorFromNonSwitchesStrings(NWildcard::CCensor &wildcardCensor, 
    const UStringVector &nonSwitchStrings, NRecursedType::EEnum type, 
    bool thereAreSwitchIncludeWildCards)
{
  int numNonSwitchStrings = nonSwitchStrings.Size();
  if(numNonSwitchStrings == kMinNonSwitchWords && (!thereAreSwitchIncludeWildCards)) 
    AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
  for(int i = kFirstFileNameIndex; i < numNonSwitchStrings; i++)
  {
    const UString &s = nonSwitchStrings[i];
    if (s[0] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, s.Mid(1), true, type);
    else
      AddCommandLineWildCardToCensr(wildcardCensor, s, true, type);
  }
}

void AddSwitchWildCardsToCensor(NWildcard::CCensor &wildcardCensor, 
    const UStringVector &strings, bool include, 
    NRecursedType::EEnum commonRecursedType)
{
  for(int i = 0; i < strings.Size(); i++)
  {
    const UString &name = strings[i];
    NRecursedType::EEnum recursedType;
    int pos = 0;
    if (name.Length() < kSomeCludePostStringMinSize)
      PrintHelpAndExit();
    if (::MyCharUpper(name[pos]) == kRecursedIDChar)
    {
      pos++;
      int index = UString(kRecursedPostCharSet).Find(name[pos]);
      recursedType = GetRecursedTypeFromIndex(index);
      if (index >= 0)
        pos++;
    }
    else
      recursedType = commonRecursedType;
    if (name.Length() < pos + kSomeCludeAfterRecursedPostStringMinSize)
      PrintHelpAndExit();
    UString tail = name.Mid(pos + 1);
    if (name[pos] == kImmediateNameID)
      AddCommandLineWildCardToCensr(wildcardCensor, tail, include, recursedType);
    else if (name[pos] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, tail, include, recursedType);
    else
      PrintHelpAndExit();
  }
}

// ------------------------------------------------------
// AddCommand functions

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

bool ParseUpdateCommandString2(const UString &command, 
    NUpdateArchive::CActionSet &actionSet, UString &postString)
{
  for(int i = 0; i < command.Length();)
  {
    char c = MyCharUpper(command[i]);
    int statePos = kUpdatePairStateIDSet.Find(c);
    if (statePos < 0)
    {
      postString = command.Mid(i);
      return true;
    }
    i++;
    if (i >= command.Length())
      return false;
    int actionPos = kUpdatePairActionIDSet.Find(::MyCharUpper(command[i]));
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
  if (dotPos >= 0 && (dotPos > slashPos || slashPos < 0))
    return name;
  return name + L'.' + extension;
}

void ParseUpdateCommandString(CUpdateArchiveOptions &options, 
    const UStringVector &updatePostStrings, 
    const NUpdateArchive::CActionSet &defaultActionSet,
    const UString &extension)
{
  for(int i = 0; i < updatePostStrings.Size(); i++)
  {
    const UString &updateString = updatePostStrings[i];
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

      UString postString;
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
        if(MyCharUpper(postString[0]) != kUpdateNewArchivePostCharID)
          PrintHelpAndExit();
        CUpdateArchiveCommand updateCommand;

        UString archivePath = postString.Mid(1);

        if (archivePath.IsEmpty())
          PrintHelpAndExit();
        updateCommand.ArchivePath = MakeFullArchiveName(archivePath, extension);
        updateCommand.ActionSet = actionSet;
        options.Commands.Add(updateCommand);
      }
    }
  }
}

static void SetAddCommandOptions(NCommandType::EEnum commandType, 
    const CParser &parser, 
    const UString &archivePath, 
    CUpdateArchiveOptions &options, UString &workingDir, 
    const UString &extension)
{
  NUpdateArchive::CActionSet defaultActionSet;
  switch(commandType)
  {
    case NCommandType::kAdd: 
      defaultActionSet = NUpdateArchive::kAddActionSet;
      break;
    case NCommandType::kDelete: 
      defaultActionSet = NUpdateArchive::kDeleteActionSet;
      break;
    default: 
      defaultActionSet = NUpdateArchive::kUpdateActionSet;
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
    const UString &postString = parser[NKey::kWorkingDir].PostStrings[0];
    if (postString.IsEmpty())
      NDirectory::MyGetTempPath(workingDir);
    else
      workingDir = postString;
  }
  else
  {
    if (!NDirectory::GetOnlyDirPrefix(archivePath, workingDir))
      throw "bad archive name";
    if (workingDir.IsEmpty())
      workingDir = L".\\";
  }
  if(options.SfxMode = parser[NKey::kSfx].ThereIs)
  {
    UString moduleName = parser[NKey::kSfx].PostStrings[0];
    if (moduleName.IsEmpty())
      moduleName = kDefaultSfxModule;
    if (!NDirectory::MySearchPath(NULL, moduleName, NULL, options.SfxModule))
      throw "can't find specified sfx module";
  }
}

static const char kByteSymbol = 'B';
static const char kKiloByteSymbol = 'K';
static const char kMegaByteSymbol = 'M';

static void SetMethodOptions(const CParser &parser, 
    CUpdateArchiveOptions &options)
{
  if (parser[NKey::kProperty].ThereIs)
  {
    // options.MethodMode.Properties.Clear();
    for(int i = 0; i < parser[NKey::kProperty].PostStrings.Size(); i++)
    {
      CProperty property;
      const UString &postString = parser[NKey::kProperty].PostStrings[i];
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

static void MyOpenArhive(const UString &archiveName, 
    const NFind::CFileInfoW &archiveFileInfo,
    #ifndef EXCLUDE_COM
    HMODULE *module,
    #endif
    IInArchive **archiveHandler,
    UString &defaultItemName,
    bool &passwordEnabled, 
    UString &password)
{
  COpenCallbackImp *openCallbackSpec = new COpenCallbackImp;
  CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
  if (passwordEnabled)
  {
    openCallbackSpec->PasswordIsDefined = passwordEnabled;
    openCallbackSpec->Password = password;
  }

  UString fullName;
  int fileNamePartStartIndex;
  NFile::NDirectory::MyGetFullPathName(archiveName, fullName, fileNamePartStartIndex);
  openCallbackSpec->LoadFileInfo(
      fullName.Left(fileNamePartStartIndex), 
      fullName.Mid(fileNamePartStartIndex));

  CArchiverInfo archiverInfo;
  int subExtIndex;
  HRESULT result = OpenArchive(archiveName, 
      #ifndef EXCLUDE_COM
      module,
      #endif
      archiveHandler, 
      archiverInfo, 
      subExtIndex,
      openCallback);
  if (result == S_FALSE)
    throw "file is not supported archive";
  if (result != S_OK)
    throw "error";
  defaultItemName = GetDefaultName(archiveName, 
      archiverInfo.Extensions[subExtIndex].Extension, 
      archiverInfo.Extensions[subExtIndex].AddExtension);
  passwordEnabled = openCallbackSpec->PasswordIsDefined;
  password = openCallbackSpec->Password;
}

#ifndef EXCLUDE_COM
void SetArchiveType(const UString &archiveType, 
    UString &filePath, CLSID &classID, UString &archiveExtension)
#else
void SetArchiveType(const UString &archiveType, 
    UString &formatName, UString &archiveExtension)
#endif
{
  CObjectVector<CArchiverInfo> archiverInfoVector;
  ReadArchiverInfoList(archiverInfoVector);
  if (archiverInfoVector.Size() == 0)
    throw "There are no installed archive handlers";
  if (archiveType.IsEmpty())
    throw "Incorrect archive type was assigned";
  for (int i = 0; i < archiverInfoVector.Size(); i++)
  {
    const CArchiverInfo &archiverInfo = archiverInfoVector[i];
    if (archiverInfo.Name.CompareNoCase(archiveType) == 0)
    {
      #ifndef EXCLUDE_COM
      classID = archiverInfo.ClassID;
      filePath = archiverInfo.FilePath;
      #else
      formatName = archiverInfo.Name;
        
      #endif

      archiveExtension = archiverInfo.GetMainExtension();
      return;
    }
  }
  throw "Incorrect archive type was assigned";
}

// int Main2(int numArguments, const char *arguments[])
int Main2()
{
  SetFileApisToOEM();
  
  g_StdOut << kCopyrightString;

  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);

  if(commandStrings.Size() == 1)
  {
    PrintHelp();
    return 0;
  }
  commandStrings.Delete(0);

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
  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

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

  UString archiveName;
  archiveName = nonSwitchStrings[kArchiveNameIndex];

  NExtractMode::EEnum extractMode;
  bool isExtractGroupCommand = command.IsFromExtractGroup(extractMode);

  bool passwordEnabled = parser[NKey::kPassword].ThereIs;

  UString password;
  if(passwordEnabled)
    password = parser[NKey::kPassword].PostStrings[0];

  if(isExtractGroupCommand || command.CommandType == NCommandType::kList)
  {
    NFind::CFileInfoW archiveFileInfo;
    if (!NFind::FindFile(archiveName, archiveFileInfo) || archiveFileInfo.IsDirectory())
      throw "there is no such archive";

   if (archiveFileInfo.IsDirectory())
      throw "there is no such archive";

    UString defaultItemName;

    #ifndef EXCLUDE_COM
    NDLL::CLibrary library;
    #endif
    CMyComPtr<IInArchive> archiveHandler;
    CArchiverInfo archiverInfo;

    MyOpenArhive(archiveName, archiveFileInfo, 
        #ifndef EXCLUDE_COM
        &library,
        #endif
        &archiveHandler, 
        defaultItemName, passwordEnabled, password);

    if(isExtractGroupCommand)
    {
      PrintProcessTitle(kExtractGroupProcessMessage, archiveName);
      UString outputDir;
      if(parser[NKey::kOutputDir].ThereIs)
      {
        outputDir = parser[NKey::kOutputDir].PostStrings[0]; // test this DirPath
        NName::NormalizeDirPathPrefix(outputDir);
      }

      NExtraction::NOverwriteMode::EEnum overwriteMode = 
          NExtraction::NOverwriteMode::kAskBefore;
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

    UString workingDir;

    UString archiveType;
    if(parser[NKey::kArchiveType].ThereIs)
      archiveType = parser[NKey::kArchiveType].PostStrings[0];
    else
      archiveType = kDefaultArchiveType;

    UString extension;
    if (!archiveType.IsEmpty())
    {
      #ifndef EXCLUDE_COM
      SetArchiveType(archiveType, options.MethodMode.FilePath, 
          options.MethodMode.ClassID1, extension);
      #else
      SetArchiveType(archiveType, options.MethodMode.Name, extension);
      #endif
    }
    if(parser[NKey::kSfx].ThereIs)
      extension = kSFXExtension;
    archiveName = MakeFullArchiveName(archiveName, extension);

    SetAddCommandOptions(command.CommandType, parser, archiveName, options,
        workingDir, extension); 
    
    SetMethodOptions(parser, options); 

    if (options.SfxMode)
    {
      CProperty property;
      property.Name = L"rsfx";
      property.Value = L"on";
      options.MethodMode.Properties.Add(property);
    }

    NFind::CFileInfoW archiveFileInfo;
    #ifndef EXCLUDE_COM
    NDLL::CLibrary library;
    #endif
    CMyComPtr<IInArchive> archive;

    UString defaultItemName;
    if (NFind::FindFile(archiveName, archiveFileInfo))
    {
      if (archiveFileInfo.IsDirectory())
        throw "there is no such archive";
      MyOpenArhive(archiveName, archiveFileInfo, 
          #ifndef EXCLUDE_COM
          &library,
          #endif
          &archive, 
          defaultItemName, passwordEnabled, password);
    }
    else
      if (archiveType.IsEmpty())
        throw "type of archive is not specified";
    bool enableParcents = !parser[NKey::kDisablePercents].ThereIs;
    if (enableParcents)
    {
      if (!isatty(fileno(stdout)))  
        enableParcents = false;
    }
    HRESULT result  = UpdateArchiveStdMain(wildcardCensor, options, workingDir, 
        archive, &defaultItemName, &archiveFileInfo, enableParcents);
    if (result != S_OK)
      throw NExitCode::CSystemError(result);
  }
  else 
    PrintHelpAndExit();
  return 0;
}
