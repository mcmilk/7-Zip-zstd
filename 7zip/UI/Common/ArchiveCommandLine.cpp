// ArchiveCommandLine.cpp

#include "StdAfx.h"

#include <io.h>
#include <stdio.h>

#include "Common/CommandLineParser.h"
#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/FileName.h"
#include "Windows/FileDir.h"
#ifdef _WIN32
#include "Windows/FileMapping.h"
#include "Windows/Synchronization.h"
#endif

#include "ArchiveCommandLine.h"
#include "UpdateAction.h"
#include "Update.h"
#include "ArchiverInfo.h"
#include "SortUtils.h"
#include "EnumDirItems.h"

using namespace NCommandLineParser;
using namespace NWindows;
using namespace NFile;

static const int kNumSwitches = 24;

namespace NKey {
enum Enum
{
  kHelp1 = 0,
  kHelp2,
  kDisableHeaders,
  kDisablePercents,
  kArchiveType,
  kYes,
  kPassword,
  kProperty,
  kOutputDir,
  kWorkingDir,
  kInclude,
  kExclude,
  kArInclude,
  kArExclude,
  kNoArName,
  kUpdate,
  kVolume,
  kRecursed,
  kSfx,
  kStdIn,
  kStdOut,
  kOverwrite,
  kEmail,
  kShowDialog
};

}


static const wchar_t kRecursedIDChar = 'R';
static const wchar_t *kRecursedPostCharSet = L"0-";

static const wchar_t *kDefaultArchiveType = L"7z";
static const wchar_t *kSFXExtension = L"exe";

namespace NRecursedPostCharIndex {
  enum EEnum 
  {
    kWildCardRecursionOnly = 0, 
    kNoRecursion = 1
  };
}

static const char kImmediateNameID = '!';
static const char kMapNameID = '#';
static const char kFileListID = '@';

static const char kSomeCludePostStringMinSize = 2; // at least <@|!><N>ame must be
static const char kSomeCludeAfterRecursedPostStringMinSize = 2; // at least <@|!><N>ame must be

static const wchar_t *kOverwritePostCharSet = L"asut";

NExtract::NOverwriteMode::EEnum k_OverwriteModes[] =
{
  NExtract::NOverwriteMode::kWithoutPrompt,
  NExtract::NOverwriteMode::kSkipExisting,
  NExtract::NOverwriteMode::kAutoRename,
  NExtract::NOverwriteMode::kAutoRenameExisting
};

static const CSwitchForm kSwitchForms[kNumSwitches] = 
  {
    { L"?",  NSwitchType::kSimple, false },
    { L"H",  NSwitchType::kSimple, false },
    { L"BA", NSwitchType::kSimple, false },
    { L"BD", NSwitchType::kSimple, false },
    { L"T",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"Y",  NSwitchType::kSimple, false },
    { L"P",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"M", NSwitchType::kUnLimitedPostString, true, 1 },
    { L"O",  NSwitchType::kUnLimitedPostString, false, 1 },
    { L"W",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"I",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"X",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AI",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AX",  NSwitchType::kUnLimitedPostString, true, kSomeCludePostStringMinSize},
    { L"AN", NSwitchType::kSimple, false },
    { L"U",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"V",  NSwitchType::kUnLimitedPostString, true, 1},
    { L"R",  NSwitchType::kPostChar, false, 0, 0, kRecursedPostCharSet },
    { L"SFX", NSwitchType::kUnLimitedPostString, false, 0 },
    { L"SI",  NSwitchType::kUnLimitedPostString, false, 0 },
    { L"SO",  NSwitchType::kSimple, false, 0 },
    { L"AO",  NSwitchType::kPostChar, false, 1, 1, kOverwritePostCharSet},
    { L"SEML", NSwitchType::kUnLimitedPostString, false, 0},
    { L"AD",  NSwitchType::kSimple, false }
  };

static const int kNumCommandForms = 7;

static const CCommandForm g_CommandForms[kNumCommandForms] = 
{
  { L"A", false },
  { L"U", false },
  { L"D", false },
  { L"T", false },
  { L"E", false },
  { L"X", false },
  { L"L", false }
};

static const int kMaxCmdLineSize = 1000;
static const wchar_t *kUniversalWildcard = L"*";
static const int kMinNonSwitchWords = 1;
static const int kCommandIndex = 0;
// static const int kArchiveNameIndex = kCommandIndex + 1;
// static const int kFirstFileNameIndex = kArchiveNameIndex + 1;

// ---------------------------
// exception messages

static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError
static const char *kIncorrectListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInListFile = "Incorrect wildcard in listfile";
static const char *kIncorrectWildCardInCommandLine  = "Incorrect wildcard in command line";

static const char *kTerminalOutError = "I won't write compressed data to a terminal";

// ---------------------------

static const AString kDefaultWorkingDirectory = "";  // test it maybemust be "."

bool CArchiveCommand::IsFromExtractGroup() const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
    case NCommandType::kExtract:
    case NCommandType::kFullExtract:
      return true;
    default:
      return false;
  }
}

NExtract::NPathMode::EEnum CArchiveCommand::GetPathMode() const
{
  switch(CommandType)
  {
    case NCommandType::kTest:
    case NCommandType::kFullExtract:
      return NExtract::NPathMode::kFullPathnames;
    default:
      return NExtract::NPathMode::kNoPathnames;
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

/*
void PrintHelp(void)
{
  g_StdErr << kHelpString;
}
*/
/*
static void ShowMessageAndThrowException(LPCSTR message, NExitCode::EEnum code)
{
  g_StdErr << message << endl;
  throw code;
}
*/

/*
static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(const AString &processTitle, const UString &archiveName)
{
  g_StdErr << endl << processTitle << 
      kProcessArchiveMessage << archiveName << endl << endl;
}
*/

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
  /*
  if (!TestIsPathLegal(name))
    throw "Can't use absolute paths";
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

static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 

void AddToCensorFromListFile(NWildcard::CCensor &wildcardCensor, 
    LPCWSTR fileName, bool include, NRecursedType::EEnum type)
{
  UStringVector names;
  if (!ReadNamesFromListFile(GetSystemString(fileName, 
        GetCurrentCodePage()), names))
    // ShowMessageAndThrowException(kIncorrectListFile, NExitCode::kUserError);
    throw kIncorrectListFile;
  for (int i = 0; i < names.Size(); i++)
    if (!AddNameToCensor(wildcardCensor, names[i], include, type))
      // ShowMessageAndThrowException(kIncorrectWildCardInListFile, NExitCode::kUserError);
      throw kIncorrectWildCardInListFile;
}

void AddCommandLineWildCardToCensr(NWildcard::CCensor &wildcardCensor, 
    const UString &name, bool include, NRecursedType::EEnum recursedType)
{
  if (!AddNameToCensor(wildcardCensor, name, include, recursedType))
    // ShowMessageAndThrowException(kIncorrectWildCardInCommandLine, NExitCode::kUserError);
    throw kIncorrectWildCardInCommandLine;
}

void AddToCensorFromNonSwitchesStrings(
    int startIndex,
    NWildcard::CCensor &wildcardCensor, 
    const UStringVector &nonSwitchStrings, NRecursedType::EEnum type, 
    bool thereAreSwitchIncludes)
{
  if(nonSwitchStrings.Size() == startIndex && (!thereAreSwitchIncludes)) 
    AddCommandLineWildCardToCensr(wildcardCensor, kUniversalWildcard, true, type);
  for(int i = startIndex; i < nonSwitchStrings.Size(); i++)
  {
    const UString &s = nonSwitchStrings[i];
    if (s[0] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, s.Mid(1), true, type);
    else
      AddCommandLineWildCardToCensr(wildcardCensor, s, true, type);
  }
}

#ifdef _WIN32
static void ParseMapWithPaths(NWildcard::CCensor &wildcardCensor, 
    const UString &switchParam, bool include, 
    NRecursedType::EEnum commonRecursedType)
{
  int splitPos = switchParam.Find(L':');
  if (splitPos < 0)
    throw kUserErrorMessage;
  UString mappingName = switchParam.Left(splitPos);
  
  UString switchParam2 = switchParam.Mid(splitPos + 1);
  splitPos = switchParam2.Find(L':');
  if (splitPos < 0)
    throw kUserErrorMessage;
  
  UString mappingSize = switchParam2.Left(splitPos);
  UString eventName = switchParam2.Mid(splitPos + 1);
  
  UInt64 dataSize64 = ConvertStringToUInt64(mappingSize, NULL);
  UInt32 dataSize = (UInt32)dataSize64;
  {
    CFileMapping fileMapping;
    if (!fileMapping.Open(FILE_MAP_READ, false, GetSystemString(mappingName)))
      throw L"Can not open mapping";
    LPVOID data = fileMapping.MapViewOfFile(FILE_MAP_READ, 0, dataSize);
    if (data == NULL)
      throw L"MapViewOfFile error";
    try
    {
      const wchar_t *curData = (const wchar_t *)data;
      if (*curData != 0)
        throw L"Incorrect mapping data";
      UInt32 numChars = dataSize / sizeof(wchar_t);
      UString name;
      for (UInt32 i = 1; i < numChars; i++)
      {
        wchar_t c = curData[i];
        if (c == L'\0')
        {
          AddCommandLineWildCardToCensr(wildcardCensor, 
              name, include, commonRecursedType);
          name.Empty();
        }
        else
          name += c;
      }
      if (!name.IsEmpty())
        throw L"data error";
    }
    catch(...)
    {
      UnmapViewOfFile(data);
      throw;
    }
    UnmapViewOfFile(data);
  }
  
  {
    NSynchronization::CEvent event;
    event.Open(EVENT_MODIFY_STATE, false, GetSystemString(eventName));
    event.Set();
  }
}
#endif

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
      // PrintHelpAndExit();
      throw kUserErrorMessage;
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
      // PrintHelpAndExit();
      throw kUserErrorMessage;
    UString tail = name.Mid(pos + 1);
    if (name[pos] == kImmediateNameID)
      AddCommandLineWildCardToCensr(wildcardCensor, tail, include, recursedType);
    else if (name[pos] == kFileListID)
      AddToCensorFromListFile(wildcardCensor, tail, include, recursedType);
    #ifdef _WIN32
    else if (name[pos] == kMapNameID)
      ParseMapWithPaths(wildcardCensor, tail, include, recursedType);
    #endif
    else
      // PrintHelpAndExit();
      throw kUserErrorMessage;
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

const UString kUpdatePairStateIDSet = L"PQRXYZW";
const int kUpdatePairStateNotSupportedActions[] = {2, 2, 1, -1, -1, -1, -1};

const UString kUpdatePairActionIDSet = L"0123"; //Ignore, Copy, Compress

const wchar_t *kUpdateIgnoreItselfPostStringID = L"-"; 
const wchar_t kUpdateNewArchivePostCharID = '!'; 


bool ParseUpdateCommandString2(const UString &command, 
    NUpdateArchive::CActionSet &actionSet, UString &postString)
{
  for(int i = 0; i < command.Length();)
  {
    wchar_t c = MyCharUpper(command[i]);
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

/*
void MakeFullArchiveName(const UString &path, CArchivePath &ap)
{
  SplitPathToParts(path, ap.Prefix, ap.Name);
  if (ap.Name.IsEmpty())
    return;
  int dotPos = ap.Name.ReverseFind(L'.');
  if (dotPos < 0)
    return;
  if (dotPos == ap.Name.Length() - 1)
  {
    ap.Name = ap.Name.Left(dotPos);
    ap.BaseExtension.Empty();
    return;
  }
  if (ap.BaseExtension.CompareNoCase(ap.Name.Mid(dotPos + 1)) == 0)
    ap.Name = ap.Name.Left(dotPos);
  else
    ap.BaseExtension.Empty();
}
*/

static void ParseUpdateCommandString(CUpdateOptions &options, 
    const UStringVector &updatePostStrings, 
    const NUpdateArchive::CActionSet &defaultActionSet)
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
        throw kUserErrorMessage;
      if(postString.IsEmpty())
      {
        if(options.UpdateArchiveItself)
          options.Commands[0].ActionSet = actionSet;
      }
      else
      {
        if(MyCharUpper(postString[0]) != kUpdateNewArchivePostCharID)
          throw kUserErrorMessage;
        CUpdateArchiveCommand uc;
        UString archivePath = postString.Mid(1);
        if (archivePath.IsEmpty())
          throw kUserErrorMessage;
        uc.ArchivePath.BaseExtension = options.ArchivePath.BaseExtension;
        uc.ArchivePath.VolExtension = options.ArchivePath.VolExtension;
        uc.ArchivePath.ParseFromPath(archivePath);
        uc.ActionSet = actionSet;
        options.Commands.Add(uc);
      }
    }
  }
}

static const char kByteSymbol = 'B';
static const char kKiloSymbol = 'K';
static const char kMegaSymbol = 'M';
static const char kGigaSymbol = 'G';

static bool ParseComplexSize(const UString &src, UInt64 &result)
{
  UString s = src;
  s.MakeUpper();

  const wchar_t *start = s;
  const wchar_t *end;
  UInt64 number = ConvertStringToUInt64(start, &end);
  int numDigits = end - start;
  if (numDigits == 0 || s.Length() > numDigits + 1)
    return false;
  if (s.Length() == numDigits)
  {
    result = number;
    return true;
  }
  int numBits;
  switch (s[numDigits])
  {
    case kByteSymbol:
      result = number;
      return true;
    case kKiloSymbol:
      numBits = 10;
      break;
    case kMegaSymbol:
      numBits = 20;
      break;
    case kGigaSymbol:
      numBits = 30;
      break;
    default:
      return false;
  }
  if (number >= ((UInt64)1 << (64 - numBits)))
    return false;
  result = number << numBits;
  return true;
}

static void SetAddCommandOptions(
    NCommandType::EEnum commandType, 
    const CParser &parser, 
    CUpdateOptions &options)
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
  
  options.UpdateArchiveItself = true;
  
  options.Commands.Clear();
  CUpdateArchiveCommand updateMainCommand;
  updateMainCommand.ActionSet = defaultActionSet;
  options.Commands.Add(updateMainCommand);
  // options.ItselfActionSet = defaultActionSet;
  if(parser[NKey::kUpdate].ThereIs)
    ParseUpdateCommandString(options, parser[NKey::kUpdate].PostStrings, 
        defaultActionSet);
  if(parser[NKey::kWorkingDir].ThereIs)
  {
    const UString &postString = parser[NKey::kWorkingDir].PostStrings[0];
    if (postString.IsEmpty())
      NDirectory::MyGetTempPath(options.WorkingDir);
    else
      options.WorkingDir = postString;
  }
  else
  {
    /*
    if (!NDirectory::GetOnlyDirPrefix(options.ArchivePath.GetFullPath(), options.WorkingDir))
      throw "bad archive name";
    if (options.WorkingDir.IsEmpty())
      options.WorkingDir = L".\\";
    */
  }
  if(options.SfxMode = parser[NKey::kSfx].ThereIs)
    options.SfxModule = parser[NKey::kSfx].PostStrings[0];

  if (parser[NKey::kVolume].ThereIs)
  {
    const UStringVector &sv = parser[NKey::kVolume].PostStrings;
    for (int i = 0; i < sv.Size(); i++)
    {
      const UString &s = sv[i];
      UInt64 size;
      if (!ParseComplexSize(sv[i], size))
        throw "incorrect volume size";
      options.VolumesSizes.Add(size);
    }
  }
}

static void SetMethodOptions(const CParser &parser, 
    CUpdateOptions &options)
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
int ParseCommandLine(UStringVector commandStrings,
    CArchiveCommandLineOptions &options)
{
  CParser parser(kNumSwitches);
  try
  {
    parser.ParseStrings(kSwitchForms, commandStrings);
  }
  catch(...) 
  {
    throw kUserErrorMessage;
  }

  options.IsInTerminal = (isatty(fileno(stdin)) != 0);
  options.IsOutTerminal = (isatty(fileno(stdout)) != 0);
  options.StdOutMode = parser[NKey::kStdOut].ThereIs;
  options.EnableHeaders = !parser[NKey::kDisableHeaders].ThereIs;
  if (options.StdOutMode)
    options.EnableHeaders = false;

  options.HelpMode = parser[NKey::kHelp1].ThereIs || parser[NKey::kHelp2].ThereIs;
  if(options.HelpMode)
  {
    return 0;
  }

  const UStringVector &nonSwitchStrings = parser.NonSwitchStrings;

  int numNonSwitchStrings = nonSwitchStrings.Size();
  if(numNonSwitchStrings < kMinNonSwitchWords)  
    throw kUserErrorMessage;

  if (!ParseArchiveCommand(nonSwitchStrings[kCommandIndex], options.Command))
    throw kUserErrorMessage;

  NRecursedType::EEnum recursedType;
  if (parser[NKey::kRecursed].ThereIs)
    recursedType = GetRecursedTypeFromIndex(parser[NKey::kRecursed].PostCharIndex);
  else
    recursedType = NRecursedType::kNonRecursed;

  bool thereAreSwitchIncludes = false;
  if (parser[NKey::kInclude].ThereIs)
  {
    thereAreSwitchIncludes = true;
    AddSwitchWildCardsToCensor(options.WildcardCensor, 
        parser[NKey::kInclude].PostStrings, true, recursedType);
  }
  if (parser[NKey::kExclude].ThereIs)
    AddSwitchWildCardsToCensor(options.WildcardCensor, 
        parser[NKey::kExclude].PostStrings, false, recursedType);
 
  int curCommandIndex = kCommandIndex + 1;
  bool thereIsArchiveName = !parser[NKey::kNoArName].ThereIs;
  if (thereIsArchiveName)
  {
    if(curCommandIndex >= numNonSwitchStrings)  
      throw kUserErrorMessage;
    options.ArchiveName = nonSwitchStrings[curCommandIndex++];
  }

  AddToCensorFromNonSwitchesStrings(
      curCommandIndex, options.WildcardCensor, 
      nonSwitchStrings, recursedType, thereAreSwitchIncludes);

  options.YesToAll = parser[NKey::kYes].ThereIs;

  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();

  options.PasswordEnabled = parser[NKey::kPassword].ThereIs;

  if(options.PasswordEnabled)
    options.Password = parser[NKey::kPassword].PostStrings[0];

  options.StdInMode = parser[NKey::kStdIn].ThereIs;
  options.ShowDialog = parser[NKey::kShowDialog].ThereIs;

  if(isExtractGroupCommand || options.Command.CommandType == NCommandType::kList)
  {
    if (options.StdInMode)
      throw "reading archives from stdin is not implemented";
    if (!options.WildcardCensor.AllAreRelative())
      throw "cannot use absolute pathnames for this command";

    NWildcard::CCensor archiveWildcardCensor;

    if (parser[NKey::kArInclude].ThereIs)
    {
      AddSwitchWildCardsToCensor(archiveWildcardCensor, 
        parser[NKey::kArInclude].PostStrings, true, NRecursedType::kNonRecursed);
    }
    if (parser[NKey::kArExclude].ThereIs)
      AddSwitchWildCardsToCensor(archiveWildcardCensor, 
      parser[NKey::kArExclude].PostStrings, false, NRecursedType::kNonRecursed);

    if (thereIsArchiveName)
      AddCommandLineWildCardToCensr(archiveWildcardCensor, options.ArchiveName, true, NRecursedType::kNonRecursed);

    CObjectVector<CDirItem> dirItems;
    EnumerateItems(archiveWildcardCensor, dirItems, NULL);
    UStringVector archivePaths;
    /*
    int pos = options.ArchiveName.ReverseFind('\\');
    UString base;
    if (pos >= 0)
      base = options.ArchiveName.Left(pos + 1);
    NFind::CEnumeratorW enumerator(options.ArchiveName);
    NFind::CFileInfoW fileInfo;
    while(enumerator.Next(fileInfo))
    {
      if (fileInfo.IsDirectory())
        continue;
      archivePaths.Add(base + fileInfo.Name);
    }
    */
    int i;
    for (i = 0; i < dirItems.Size(); i++)
      archivePaths.Add(dirItems[i].FullPath);

    if (archivePaths.Size() == 0)
      throw "there is no such archive";

    UStringVector archivePathsFull;

    for (i = 0; i < archivePaths.Size(); i++)
    {
      UString fullPath;
      NFile::NDirectory::MyGetFullPathName(archivePaths[i], fullPath);
      archivePathsFull.Add(fullPath);
    }
    CIntVector indices;
    SortStringsToIndices(archivePathsFull, indices);
    options.ArchivePathsSorted.Reserve(indices.Size());
    options.ArchivePathsFullSorted.Reserve(indices.Size());
    for (i = 0; i < indices.Size(); i++)
    {
      options.ArchivePathsSorted.Add(archivePaths[indices[i]]);
      options.ArchivePathsFullSorted.Add(archivePathsFull[indices[i]]);
    }

    if(isExtractGroupCommand)
    {
      if (options.StdOutMode && options.IsOutTerminal)
        throw kTerminalOutError;
      if(parser[NKey::kOutputDir].ThereIs)
      {
        options.OutputDir = parser[NKey::kOutputDir].PostStrings[0];
        NFile::NName::NormalizeDirPathPrefix(options.OutputDir);
      }

      options.OverwriteMode = NExtract::NOverwriteMode::kAskBefore;
      if(parser[NKey::kOverwrite].ThereIs)
        options.OverwriteMode = 
            k_OverwriteModes[parser[NKey::kOverwrite].PostCharIndex];
      else if (options.YesToAll)
        options.OverwriteMode = NExtract::NOverwriteMode::kWithoutPrompt;
    }
  }
  else if(options.Command.IsFromUpdateGroup())
  {
    CUpdateOptions &updateOptions = options.UpdateOptions;

    UString archiveType;
    if(parser[NKey::kArchiveType].ThereIs)
      archiveType = parser[NKey::kArchiveType].PostStrings[0];
    else
      archiveType = kDefaultArchiveType;

    UString typeExtension;
    if (!archiveType.IsEmpty())
    {
      #ifndef EXCLUDE_COM
      SetArchiveType(archiveType, updateOptions.MethodMode.FilePath, 
          updateOptions.MethodMode.ClassID, typeExtension);
      #else
      SetArchiveType(archiveType, updateOptions.MethodMode.Name, typeExtension);
      #endif
    }
    UString extension = typeExtension;
    if(parser[NKey::kSfx].ThereIs)
      extension = kSFXExtension;
    updateOptions.ArchivePath.BaseExtension = extension;
    updateOptions.ArchivePath.VolExtension = typeExtension;
    updateOptions.ArchivePath.ParseFromPath(options.ArchiveName);
    SetAddCommandOptions(options.Command.CommandType, parser, 
        updateOptions); 
    
    SetMethodOptions(parser, updateOptions); 

    /*
    if (updateOptions.SfxMode)
    {
      CProperty property;
      property.Name = L"rsfx";
      property.Value = L"on";
      updateOptions.MethodMode.Properties.Add(property);
    }
    */

    ///
    
    options.EnablePercents = !parser[NKey::kDisablePercents].ThereIs;

    if (options.EnablePercents)
    {
      /*
      if (!options.IsTerminal)  
        options.EnablePercents = false;
      */
    }

    if (updateOptions.EMailMode = parser[NKey::kEmail].ThereIs)
    {
      updateOptions.EMailAddress = parser[NKey::kEmail].PostStrings.Front();
      if (updateOptions.EMailAddress.Length() > 0)
        if (updateOptions.EMailAddress[0] == L'.')
        {
          updateOptions.EMailRemoveAfter = true;
          updateOptions.EMailAddress.Delete(0);
        }
    }

    updateOptions.StdOutMode = options.StdOutMode;
    updateOptions.StdInMode = options.StdInMode;

    if (updateOptions.StdOutMode && updateOptions.EMailMode)
      throw "stdout and email cannot be combined";
    
    if (updateOptions.StdOutMode && options.IsOutTerminal)
      throw kTerminalOutError;
    if(updateOptions.StdInMode)
      updateOptions.StdInFileName = parser[NKey::kStdIn].PostStrings.Front();

  }
  else 
    throw kUserErrorMessage;
  return 0;
}


